#include <fstream>
#include <gpiod.h>
#include <mutex>
#include <signal.h>
#include <string>
#include <thread>
#include <unistd.h>	// usleep

//---------------------------- Configuration constants -------------------------

// Modify the following to match the desired output GPIO
const char *chipname = "gpiochip1";
const int gpio_line = 98;

// PWM frequency
const float freq = 100;

// Target CPU temperature - in degrees C
const float target = 45;

// Minimum duty cycle (range 0-1)
// Some fans have trouble starting if the duty cycle is too low
const float min_d = 0.3;

// Duty cycle step size
// Note that smaller steps will cause slower fan reaction
// Smaller steps are advisable with larger heatsinks.
const float d_step = 0.05;

//--------------------------------- Misc stuff ---------------------------------

std::mutex pwm_data_mutex;
float pwm_data = 1;
bool pwm_closing = 0;
bool main_closing = 0;

//---------------------------- Temperature sensing -----------------------------


float fetchTemp() {
	// Returns CPU temperature in degrees C

	const std::string temp_file_name = 
			"/sys/devices/virtual/thermal/thermal_zone0/temp";

	std::ifstream temp_file(temp_file_name);

	if(!temp_file.is_open()) {
          	std::fprintf(stderr,"Unable to open %s.\n",
						temp_file_name.c_str());
		return -1.0;
	}

	float result;
	temp_file >> result;
	temp_file.close();
	result /= 1000;
	return result;	
}

//------------------------------------ PWM loop --------------------------------

void PWM() {
	// This is a process intended to run as a separate thread
	// for the sake of simplicity. Really.
	// It reads pwm_data and executes whatever is in there.
	// In order to kill this thread gracefully, set "pwm_closing" to 1. 
	struct gpiod_chip *chip;
	struct gpiod_line *fan;
	chip = gpiod_chip_open_by_name(chipname);
	fan = gpiod_chip_get_line(chip, gpio_line);
	gpiod_line_request_output(fan, "PWM Fan", 0);

	// PWM data in (0 - 1) range
	float local_pwm_data = 0;
	// On time in current PWM cycle
	float on_time = 0;
	// PWM period in us
	float period = 1000000 / freq;

	while (!pwm_closing) {

		if (pwm_data_mutex.try_lock()) {
			local_pwm_data = pwm_data;
			pwm_data_mutex.unlock();
			on_time = local_pwm_data * period;
		}

		if (on_time > 0) gpiod_line_set_value(fan, 1);
		usleep(on_time);
		gpiod_line_set_value(fan, 0);
		usleep(period - on_time);
	}

	gpiod_line_release(fan);
	gpiod_chip_close(chip);
}

//--------------------------------- Signal Handle ------------------------------

void signal_handle(const int s) {
	// Handles a few POSIX signals, asking the process to die gracefully
	
	main_closing = 1;

	if (s){};	// Suppress warning about unused, mandatory parameter
}

//----------------------------------- Main Loop --------------------------------

int main(int argc, char **argv)
{

	signal (SIGINT, signal_handle);		// Catches SIGINT (ctrl+c)
	signal (SIGTERM, signal_handle);	// Catches SIGTERM

	// Create PWM thread
	std::thread pwm_thread (PWM);

	// Assign Real Time priority to PWM thread
	sched_param sch;
	int policy;
	pthread_getschedparam(pwm_thread.native_handle(), &policy, &sch);
	sch.sched_priority = 99;
	pthread_setschedparam(pwm_thread.native_handle(), SCHED_FIFO, &sch);

	float current;
	float d = 1;

	while (!main_closing) {
		usleep(100000);
		current = fetchTemp();

		if (current < target)  d -= d_step;
		if (current > target)  d += d_step;

		if (d > 1) d = 1;
		if (d < 0) d = 0;

		pwm_data_mutex.lock();
		pwm_data = (d == 0) ? 0 : (1 - min_d) * d + min_d;
		pwm_data_mutex.unlock();
	}

	pwm_closing = 1;
	pwm_thread.join();
	exit(0);
}
