#!/usr/bin/env python3
"""Build synchronized multimodal visualization video from [name].csv and [name].mp4."""

from __future__ import annotations

import argparse
import csv
import math
import re
import shutil
import subprocess
import sys
import wave
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

import cv2
import numpy as np
from scipy.signal import stft

THERMAL_ROWS = 24
THERMAL_COLS = 32
THERMAL_PIXELS = THERMAL_ROWS * THERMAL_COLS
AUDIO_SAMPLE_RATE = 16000
IMU_AXES_TO_PLOT = 6  # ax, ay, az, gx, gy, gz

IMU_COLORS_BGR = [
    (255, 212, 0),
    (255, 160, 0),
    (255, 100, 80),
    (255, 80, 160),
    (200, 80, 255),
    (120, 160, 255),
]


@dataclass
class ImuSample:
    t_ms: float
    values: np.ndarray  # shape=(9,)
    device_us: Optional[float] = None


@dataclass
class AudioPacket:
    t_ms: float
    missing_samples: int
    samples: np.ndarray


@dataclass
class IrFrame:
    t_ms: float
    pixels: np.ndarray  # shape=(768,)
    device_us: Optional[float] = None


@dataclass
class CsvData:
    imu: List[ImuSample]
    audio: List[AudioPacket]
    ir: List[IrFrame]
    video_start_ms: Optional[float]
    video_end_ms: Optional[float]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Visualize synchronized webcam/IR/IMU/audio from [name].csv and [name].mp4."
    )
    parser.add_argument(
        "name",
        help="Base name or path prefix. Example: inhaler -> inhaler.csv + inhaler.mp4",
    )
    parser.add_argument(
        "--fps",
        type=float,
        default=None,
        help="Output FPS (default: use webcam FPS or 25).",
    )
    return parser.parse_args()


def resolve_paths(name_arg: str) -> Tuple[Path, Path, Path]:
    base = Path(name_arg)
    if base.suffix:
        base = base.with_suffix("")
    csv_path = base.with_suffix(".csv")
    mp4_path = base.with_suffix(".mp4")
    out_path = base.with_name(f"visualize_{base.name}.mp4")
    return csv_path, mp4_path, out_path


def _safe_float(text: str, default: float = float("nan")) -> float:
    try:
        return float(text)
    except Exception:
        return default


def _safe_int(text: str, default: int = 0) -> int:
    try:
        return int(float(text))
    except Exception:
        return default


def parse_format_number(fmt: str, key: str) -> Optional[float]:
    m = re.search(rf"(?:^|;){re.escape(key)}=([-+]?\d+(?:\.\d+)?)", fmt)
    if not m:
        return None
    return _safe_float(m.group(1), float("nan"))


def parse_csv_data(csv_path: Path) -> CsvData:
    imu: List[ImuSample] = []
    audio: List[AudioPacket] = []
    ir: List[IrFrame] = []
    video_start_ms: Optional[float] = None
    video_end_ms: Optional[float] = None

    with csv_path.open("r", encoding="utf-8", newline="") as f:
        reader = csv.reader(f)
        header = next(reader, None)
        if not header:
            raise ValueError(f"CSV is empty: {csv_path}")

        for row in reader:
            if len(row) < 3:
                continue
            t_ms = _safe_float(row[0])
            row_type = row[1].strip().upper()
            fmt_or_marker = row[2].strip().lower()

            if row_type == "VIDEO":
                if fmt_or_marker == "start":
                    video_start_ms = t_ms
                elif fmt_or_marker == "end":
                    video_end_ms = t_ms
                continue

            if row_type == "IMU":
                values = np.array([_safe_float(x, 0.0) for x in row[3:12]], dtype=np.float32)
                if values.size == 9 and np.isfinite(t_ms):
                    imu.append(ImuSample(t_ms=t_ms, values=values, device_us=parse_format_number(fmt_or_marker, "device_us")))
                continue

            if row_type == "AUDIO":
                missing = 0
                m = re.search(r"missing_samples=(\d+)", fmt_or_marker)
                if m:
                    missing = int(m.group(1))
                samples = np.array([_safe_int(x, 0) for x in row[3:]], dtype=np.int16)
                if samples.size > 0 and np.isfinite(t_ms):
                    audio.append(AudioPacket(t_ms=t_ms, missing_samples=missing, samples=samples))
                continue

            if row_type == "IR":
                pixels = np.array([_safe_int(x, 0) for x in row[3 : 3 + THERMAL_PIXELS]], dtype=np.int16)
                if pixels.size == THERMAL_PIXELS and np.isfinite(t_ms):
                    ir.append(IrFrame(t_ms=t_ms, pixels=pixels, device_us=parse_format_number(fmt_or_marker, "device_us")))

    imu.sort(key=lambda x: x.t_ms)
    audio.sort(key=lambda x: x.t_ms)
    ir.sort(key=lambda x: x.t_ms)
    if ir and ir[0].device_us is not None:
        anchor_device_us = ir[0].device_us
        anchor_wall_ms = ir[0].t_ms
        for sample in imu:
            if sample.device_us is not None and np.isfinite(sample.device_us):
                sample.t_ms = anchor_wall_ms + (sample.device_us - anchor_device_us) / 1000.0
        for frame in ir:
            if frame.device_us is not None and np.isfinite(frame.device_us):
                frame.t_ms = anchor_wall_ms + (frame.device_us - anchor_device_us) / 1000.0
        imu.sort(key=lambda x: x.t_ms)
        ir.sort(key=lambda x: x.t_ms)
    return CsvData(imu=imu, audio=audio, ir=ir, video_start_ms=video_start_ms, video_end_ms=video_end_ms)


def reconstruct_audio_timeline(audio_packets: Sequence[AudioPacket]) -> Tuple[np.ndarray, Optional[float], Optional[float]]:
    if not audio_packets:
        return np.zeros(0, dtype=np.float32), None, None
    audio_start = float(audio_packets[0].t_ms)
    chunks: List[np.ndarray] = []
    for pkt in audio_packets:
        if pkt.missing_samples > 0:
            chunks.append(np.zeros(pkt.missing_samples, dtype=np.float32))
        chunks.append(pkt.samples.astype(np.float32))
    signal = np.concatenate(chunks) if chunks else np.zeros(0, dtype=np.float32)
    audio_end = audio_start + (signal.size / AUDIO_SAMPLE_RATE) * 1000.0
    return signal, audio_start, audio_end


def colorize_ir_frame(pixels: np.ndarray, out_w: int, out_h: int) -> np.ndarray:
    frame = pixels.reshape((THERMAL_ROWS, THERMAL_COLS)).astype(np.float32)
    frame = frame / 50.0  # raw to Celsius-like scale used in web preview
    min_v = float(np.min(frame))
    max_v = float(np.max(frame))
    if math.isclose(min_v, max_v):
        norm = np.zeros_like(frame, dtype=np.uint8)
    else:
        norm = ((frame - min_v) / (max_v - min_v) * 255.0).astype(np.uint8)
    colored = cv2.applyColorMap(norm, cv2.COLORMAP_INFERNO)
    return cv2.resize(colored, (out_w, out_h), interpolation=cv2.INTER_NEAREST)


def make_imu_panel(
    imu_samples: Sequence[ImuSample],
    global_start_ms: float,
    global_end_ms: float,
    width: int,
    height: int,
) -> np.ndarray:
    panel = np.zeros((height, width, 3), dtype=np.uint8)
    panel[:] = (12, 18, 26)

    # The IMU plot must use the exact same horizontal pixel timeline as the
    # audio spectrogram and progress cursor. Do not add left/right padding here.
    pad_l, pad_r, pad_t, pad_b = 0, 0, 20, 20
    plot_w = max(10, width - pad_l - pad_r)
    plot_h = max(10, height - pad_t - pad_b)
    cv2.rectangle(panel, (pad_l, pad_t), (pad_l + plot_w, pad_t + plot_h), (45, 60, 78), 1)

    for i in range(1, 5):
        y = pad_t + int(i * plot_h / 5)
        cv2.line(panel, (pad_l, y), (pad_l + plot_w, y), (30, 42, 56), 1)

    if not imu_samples:
        cv2.putText(panel, "No IMU data", (pad_l + 8, pad_t + 28), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (160, 170, 190), 2)
        return panel

    # Keep timestamps as float64. Absolute wall-clock milliseconds are ~1e12,
    # and float32 loses sub-second precision at that scale, collapsing x positions.
    t = np.array([s.t_ms for s in imu_samples], dtype=np.float64)
    values = np.stack([s.values[:IMU_AXES_TO_PLOT] for s in imu_samples], axis=0).astype(np.float32)

    for axis in range(IMU_AXES_TO_PLOT):
        axis_values = values[:, axis].astype(np.float32)
        lo = float(np.percentile(axis_values, 1))
        hi = float(np.percentile(axis_values, 99))
        center = (lo + hi) / 2.0
        scale = max((hi - lo) / 2.0, 1e-6)
        y_norm = np.clip((axis_values - center) / scale, -1.0, 1.0)
        rel_t = (t - global_start_ms) / max(1e-6, (global_end_ms - global_start_ms))
        x = (np.clip(rel_t, 0.0, 1.0) * (width - 1)).astype(np.int32)
        y = pad_t + ((1.0 - (y_norm * 0.48 + 0.5)) * plot_h).astype(np.int32)
        pts = np.stack([x, y], axis=1).reshape(-1, 1, 2)
        cv2.polylines(panel, [pts], isClosed=False, color=IMU_COLORS_BGR[axis], thickness=2, lineType=cv2.LINE_AA)

    cv2.putText(panel, "IMU (ax, ay, az, gx, gy, gz)", (12, 16), cv2.FONT_HERSHEY_SIMPLEX, 0.56, (205, 215, 230), 1)
    return panel


def make_audio_spec_panel(
    audio_signal: np.ndarray,
    global_start_ms: float,
    global_end_ms: float,
    audio_start_ms: Optional[float],
    width: int,
    height: int,
) -> np.ndarray:
    panel = np.zeros((height, width, 3), dtype=np.uint8)
    panel[:] = (12, 18, 26)

    if audio_signal.size == 0 or audio_start_ms is None:
        cv2.putText(panel, "No audio data", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (160, 170, 190), 2)
        return panel

    pre = int(round((audio_start_ms - global_start_ms) / 1000.0 * AUDIO_SAMPLE_RATE))
    pre = max(0, pre)
    total = int(round((global_end_ms - global_start_ms) / 1000.0 * AUDIO_SAMPLE_RATE))
    total = max(total, pre + audio_signal.size)
    padded = np.zeros(total, dtype=np.float32)
    end = min(total, pre + audio_signal.size)
    padded[pre:end] = audio_signal[: end - pre]

    _, _, zxx = stft(padded, fs=AUDIO_SAMPLE_RATE, nperseg=512, noverlap=384, boundary=None, padded=False)
    mag = np.abs(zxx)
    db = 20.0 * np.log10(np.maximum(mag, 1e-6))
    db -= float(np.max(db))
    db = np.clip(db, -80.0, 0.0)
    norm = ((db + 80.0) / 80.0 * 255.0).astype(np.uint8)
    spec = cv2.applyColorMap(np.flipud(norm), cv2.COLORMAP_INFERNO)
    spec = cv2.resize(spec, (width, height), interpolation=cv2.INTER_LINEAR)

    cv2.putText(spec, "Audio STFT", (12, 22), cv2.FONT_HERSHEY_SIMPLEX, 0.62, (235, 235, 235), 2)
    return spec


def make_aligned_audio_signal(
    audio_signal: np.ndarray,
    audio_start_ms: Optional[float],
    global_start_ms: float,
    global_end_ms: float,
) -> np.ndarray:
    total = int(math.ceil((global_end_ms - global_start_ms) / 1000.0 * AUDIO_SAMPLE_RATE))
    total = max(total, 0)
    aligned = np.zeros(total, dtype=np.float32)
    if audio_signal.size == 0 or audio_start_ms is None or total == 0:
        return aligned

    start = int(round((audio_start_ms - global_start_ms) / 1000.0 * AUDIO_SAMPLE_RATE))
    src_start = max(0, -start)
    dst_start = max(0, start)
    n = min(audio_signal.size - src_start, total - dst_start)
    if n > 0:
        aligned[dst_start : dst_start + n] = audio_signal[src_start : src_start + n]
    return aligned


def write_pcm16_wav(path: Path, samples: np.ndarray, sample_rate: int = AUDIO_SAMPLE_RATE) -> None:
    pcm = np.clip(np.round(samples), -32768, 32767).astype(np.int16)
    with wave.open(str(path), "wb") as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        wf.writeframes(pcm.tobytes())


def find_ffmpeg_executable() -> Optional[str]:
    exe = shutil.which("ffmpeg")
    if exe:
        return exe
    try:
        import imageio_ffmpeg  # type: ignore

        return imageio_ffmpeg.get_ffmpeg_exe()
    except Exception:
        return None


def mux_audio_into_video(silent_video_path: Path, audio_wav_path: Path, output_path: Path) -> bool:
    ffmpeg_exe = find_ffmpeg_executable()
    if not ffmpeg_exe:
        print("Warning: ffmpeg not found; saved video without embedded audio.")
        print("Install with: pip install imageio-ffmpeg")
        return False
    cmd = [
        ffmpeg_exe,
        "-y",
        "-i",
        str(silent_video_path),
        "-i",
        str(audio_wav_path),
        "-map",
        "0:v:0",
        "-map",
        "1:a:0",
        "-c:v",
        "copy",
        "-c:a",
        "aac",
        "-b:a",
        "128k",
        "-shortest",
        "-movflags",
        "+faststart",
        str(output_path),
    ]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print("Warning: ffmpeg failed; saved video without embedded audio.")
        print(result.stderr[-2000:])
        return False
    return True


def save_modal_timeline_figure(
    out_path: Path,
    global_start_ms: float,
    global_end_ms: float,
    video_range: Tuple[Optional[float], Optional[float]],
    ir_range: Tuple[Optional[float], Optional[float]],
    imu_range: Tuple[Optional[float], Optional[float]],
    audio_range: Tuple[Optional[float], Optional[float]],
) -> None:
    width, height = 1400, 420
    pad_l, pad_r, pad_t, pad_b = 120, 40, 30, 70
    plot_w = width - pad_l - pad_r
    plot_h = height - pad_t - pad_b
    y_min, y_max = 0.5, 4.5

    canvas = np.full((height, width, 3), 255, dtype=np.uint8)
    cv2.rectangle(canvas, (pad_l, pad_t), (pad_l + plot_w, pad_t + plot_h), (200, 200, 200), 1)

    def x_of(t_ms: float) -> int:
        u = (t_ms - global_start_ms) / max(1e-6, (global_end_ms - global_start_ms))
        return int(round(pad_l + np.clip(u, 0.0, 1.0) * plot_w))

    def y_of(v: float) -> int:
        u = (v - y_min) / (y_max - y_min)
        return int(round(pad_t + (1.0 - u) * plot_h))

    # Horizontal guide lines and y labels
    labels = [(1, "video"), (2, "ir"), (3, "imu"), (4, "audio")]
    for y_val, label in labels:
        yy = y_of(float(y_val))
        cv2.line(canvas, (pad_l, yy), (pad_l + plot_w, yy), (230, 230, 230), 1)
        cv2.putText(canvas, f"y={y_val} ({label})", (16, yy + 5), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (70, 70, 70), 1)

    def draw_segment(t0: Optional[float], t1: Optional[float], y_val: int, color: Tuple[int, int, int]) -> None:
        if t0 is None or t1 is None:
            return
        x0 = x_of(float(t0))
        x1 = x_of(float(t1))
        yy = y_of(float(y_val))
        cv2.line(canvas, (x0, yy), (x1, yy), color, 6, lineType=cv2.LINE_AA)

    draw_segment(video_range[0], video_range[1], 1, (0, 120, 255))
    draw_segment(ir_range[0], ir_range[1], 2, (60, 60, 220))
    draw_segment(imu_range[0], imu_range[1], 3, (0, 170, 0))
    draw_segment(audio_range[0], audio_range[1], 4, (180, 0, 180))

    # x-axis ticks
    n_ticks = 8
    for i in range(n_ticks + 1):
        u = i / n_ticks
        x = int(round(pad_l + u * plot_w))
        cv2.line(canvas, (x, pad_t + plot_h), (x, pad_t + plot_h + 8), (90, 90, 90), 1)
        t_ms = global_start_ms + u * (global_end_ms - global_start_ms)
        t_s = (t_ms - global_start_ms) / 1000.0
        cv2.putText(
            canvas,
            f"{t_s:.3f}",
            (x - 22, pad_t + plot_h + 28),
            cv2.FONT_HERSHEY_SIMPLEX,
            0.48,
            (80, 80, 80),
            1,
        )
    cv2.putText(canvas, "relative_time_s (t0=0)", (pad_l + plot_w - 230, pad_t + plot_h + 50), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (70, 70, 70), 1)
    cv2.putText(canvas, "Modal start/end timeline", (pad_l, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (50, 50, 50), 2)

    cv2.imwrite(str(out_path), canvas)


def main() -> None:
    args = parse_args()
    csv_path, mp4_path, out_path = resolve_paths(args.name)

    if not csv_path.exists():
        raise FileNotFoundError(f"CSV not found: {csv_path}")
    if not mp4_path.exists():
        raise FileNotFoundError(f"Video not found: {mp4_path}")

    data = parse_csv_data(csv_path)
    audio_signal, audio_start_ms, audio_end_ms = reconstruct_audio_timeline(data.audio)

    cap = cv2.VideoCapture(str(mp4_path))
    if not cap.isOpened():
        raise RuntimeError(f"Cannot open webcam video: {mp4_path}")

    cam_fps = cap.get(cv2.CAP_PROP_FPS)
    cam_fps = cam_fps if cam_fps and cam_fps > 1e-3 else 25.0
    out_fps = args.fps if args.fps and args.fps > 1e-3 else cam_fps
    cam_w = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)) or 640
    cam_h = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT)) or 480
    cam_frame_count = int(cap.get(cv2.CAP_PROP_FRAME_COUNT)) or 0
    cam_duration_ms = (cam_frame_count / cam_fps) * 1000.0 if cam_frame_count > 0 else 0.0

    video_start_ms = data.video_start_ms
    video_end_ms = data.video_end_ms
    if video_start_ms is None:
        candidates = []
        if data.imu:
            candidates.append(data.imu[0].t_ms)
        if data.ir:
            candidates.append(data.ir[0].t_ms)
        if audio_start_ms is not None:
            candidates.append(audio_start_ms)
        video_start_ms = min(candidates) if candidates else 0.0
    if video_end_ms is None:
        video_end_ms = video_start_ms + cam_duration_ms

    imu_start = data.imu[0].t_ms if data.imu else None
    imu_end = data.imu[-1].t_ms if data.imu else None
    ir_start = data.ir[0].t_ms if data.ir else None
    ir_end = data.ir[-1].t_ms if data.ir else None

    starts = [video_start_ms]
    ends = [video_end_ms]
    if data.imu:
        starts.append(imu_start)
        ends.append(imu_end)
    if data.ir:
        starts.append(ir_start)
        ends.append(ir_end)
    if audio_start_ms is not None and audio_end_ms is not None:
        starts.append(audio_start_ms)
        ends.append(audio_end_ms)

    global_start_ms = float(min(starts))
    global_end_ms = float(max(ends))
    if global_end_ms <= global_start_ms:
        global_end_ms = global_start_ms + 1000.0

    def _print_range(name: str, start: Optional[float], end: Optional[float], count: Optional[int] = None) -> None:
        if start is None or end is None:
            print(f"{name}: no data")
            return
        suffix = f", count={count}" if count is not None else ""
        print(f"{name}: start={(start - global_start_ms) / 1000.0:.3f}s, end={(end - global_start_ms) / 1000.0:.3f}s, duration={(end - start) / 1000.0:.3f}s{suffix}")

    print("Modality ranges relative to first data:")
    _print_range("video", video_start_ms, video_end_ms)
    _print_range("ir", ir_start, ir_end, len(data.ir))
    _print_range("imu", imu_start, imu_end, len(data.imu))
    _print_range("audio", audio_start_ms, audio_end_ms, len(data.audio))

    timeline_path = out_path.with_name(f"visualize_{out_path.stem.replace('visualize_', '')}_timeline.png")
    save_modal_timeline_figure(
        timeline_path,
        global_start_ms,
        global_end_ms,
        video_range=(video_start_ms, video_end_ms),
        ir_range=(ir_start, ir_end),
        imu_range=(imu_start, imu_end),
        audio_range=(audio_start_ms, audio_end_ms),
    )

    top_w = cam_w
    top_h = cam_h
    full_w = top_w * 2
    imu_h = 320
    spec_h = 320
    full_h = top_h + imu_h + spec_h

    imu_panel_base = make_imu_panel(data.imu, global_start_ms, global_end_ms, full_w, imu_h)
    spec_panel_base = make_audio_spec_panel(audio_signal, global_start_ms, global_end_ms, audio_start_ms, full_w, spec_h)

    silent_video_path = out_path.with_name(f"{out_path.stem}_silent_tmp.mp4")
    audio_wav_path = out_path.with_name(f"{out_path.stem}_audio_tmp.wav")

    writer = cv2.VideoWriter(
        str(silent_video_path),
        cv2.VideoWriter_fourcc(*"mp4v"),
        out_fps,
        (full_w, full_h),
    )
    if not writer.isOpened():
        raise RuntimeError(f"Cannot open output writer: {silent_video_path}")

    black_cam = np.zeros((top_h, top_w, 3), dtype=np.uint8)
    black_ir = np.zeros((top_h, top_w, 3), dtype=np.uint8)

    cam_last_frame = black_cam.copy()
    cam_last_rel_ms = -1.0
    cam_read_idx = 0
    cam_eof = False
    ir_idx = 0

    n_frames = int(math.ceil((global_end_ms - global_start_ms) / 1000.0 * out_fps)) + 1
    progress_step = max(1, n_frames // 200)  # about 0.5% refresh
    for i in range(n_frames):
        t_ms = global_start_ms + i * (1000.0 / out_fps)

        # Webcam frame: strict timestamp mapping using decoded frame timestamps.
        if video_start_ms <= t_ms <= video_end_ms and cam_frame_count > 0:
            target_rel_ms = t_ms - video_start_ms
            while not cam_eof and cam_last_rel_ms < target_rel_ms:
                ok, frame = cap.read()
                if not ok:
                    cam_eof = True
                    break
                cam_read_idx += 1
                rel_ms = cap.get(cv2.CAP_PROP_POS_MSEC)
                # Some backends may return 0/NaN for POS_MSEC; fall back to FPS timeline.
                if not np.isfinite(rel_ms) or (rel_ms <= 0.0 and cam_read_idx > 1):
                    rel_ms = (cam_read_idx - 1) * (1000.0 / cam_fps)
                cam_last_rel_ms = float(rel_ms)
                cam_last_frame = cv2.resize(frame, (top_w, top_h), interpolation=cv2.INTER_LINEAR)

            if cam_read_idx == 0:
                cam_frame = black_cam
            elif cam_eof and target_rel_ms > cam_last_rel_ms:
                # Outside real webcam coverage: zero-pad (black frame).
                cam_frame = black_cam
            else:
                cam_frame = cam_last_frame
        else:
            cam_frame = black_cam

        # IR frame (hold-last inside range, black outside)
        while ir_idx + 1 < len(data.ir) and data.ir[ir_idx + 1].t_ms <= t_ms:
            ir_idx += 1
        if data.ir and data.ir[0].t_ms <= t_ms <= data.ir[-1].t_ms:
            ir_frame = colorize_ir_frame(data.ir[ir_idx].pixels, top_w, top_h)
        else:
            ir_frame = black_ir

        # Row 2/3 with progress cursor
        x_line = int(round((t_ms - global_start_ms) / max(1e-6, (global_end_ms - global_start_ms)) * (full_w - 1)))
        x_line = max(0, min(full_w - 1, x_line))
        imu_panel = imu_panel_base.copy()
        spec_panel = spec_panel_base.copy()
        cv2.line(imu_panel, (x_line, 0), (x_line, imu_h - 1), (0, 255, 255), 2)
        cv2.line(spec_panel, (x_line, 0), (x_line, spec_h - 1), (0, 255, 255), 2)

        # Compose frame
        canvas = np.zeros((full_h, full_w, 3), dtype=np.uint8)
        canvas[:top_h, :top_w] = cam_frame
        canvas[:top_h, top_w:] = ir_frame
        canvas[top_h : top_h + imu_h, :] = imu_panel
        canvas[top_h + imu_h :, :] = spec_panel

        cv2.putText(canvas, "Webcam", (14, 26), cv2.FONT_HERSHEY_SIMPLEX, 0.72, (255, 255, 255), 2)
        cv2.putText(canvas, "IR", (top_w + 14, 26), cv2.FONT_HERSHEY_SIMPLEX, 0.72, (255, 255, 255), 2)
        cv2.putText(canvas, f"t = {t_ms:.3f} ms", (14, full_h - 12), cv2.FONT_HERSHEY_SIMPLEX, 0.62, (220, 220, 220), 2)

        writer.write(canvas)

        if i == 0 or (i + 1) % progress_step == 0 or (i + 1) == n_frames:
            done = i + 1
            pct = done * 100.0 / max(1, n_frames)
            bar_w = 30
            fill = int(round((done / max(1, n_frames)) * bar_w))
            bar = "#" * fill + "-" * (bar_w - fill)
            sys.stdout.write(f"\rRendering [{bar}] {done}/{n_frames} ({pct:5.1f}%)")
            sys.stdout.flush()

    cap.release()
    writer.release()

    aligned_audio = make_aligned_audio_signal(audio_signal, audio_start_ms, global_start_ms, global_end_ms)
    if aligned_audio.size > 0 and np.any(aligned_audio):
        write_pcm16_wav(audio_wav_path, aligned_audio)
        if not mux_audio_into_video(silent_video_path, audio_wav_path, out_path):
            silent_video_path.replace(out_path)
    else:
        silent_video_path.replace(out_path)

    for tmp_path in (silent_video_path, audio_wav_path):
        if tmp_path.exists():
            try:
                tmp_path.unlink()
            except OSError:
                pass

    sys.stdout.write("\n")
    print(f"Saved: {out_path}")
    print(f"Saved: {timeline_path}")


if __name__ == "__main__":
    main()
