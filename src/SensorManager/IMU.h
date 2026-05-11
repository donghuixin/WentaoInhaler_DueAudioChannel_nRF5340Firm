#ifndef IMU_H
#define IMU_H

#include "EdgeMLSensor.h"

#include "openearable_common.h"

class IMU : public EdgeMlSensor {
public:
    static IMU sensor;

    bool init(struct k_msgq * queue) override;
    void start(int sample_rate_idx) override;
    void stop() override;

    const static SampleRateSetting<6> sample_rates;
private:
    static void sensor_timer_handler(struct k_timer *dummy);

    static void update_sensor(struct k_work *work);
    bool _active = false;
};

#endif
