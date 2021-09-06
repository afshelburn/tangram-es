/*
 * test.cpp
 *
 *  Created on: Sep 3, 2021
 *      Author: anthony
 */

#include <iostream>
#include <queue>
#include <pigpiod_if2.h>
#include <optional>
#include <vector>
#include <mutex>
#include <thread>
#include <chrono>
#include <algorithm>
#include <memory>
#include <map>
#include <vector>

template<typename T>
class ThreadsafeQueue {
	std::queue<T> queue_;
	mutable std::mutex mutex_;

	// Moved out of public interface to prevent races between this
	// and pop().
	bool empty() const {
		return queue_.empty();
	}

public:
	ThreadsafeQueue() = default;
	ThreadsafeQueue(const ThreadsafeQueue<T>&) = delete;
	ThreadsafeQueue& operator=(const ThreadsafeQueue<T>&) = delete;

	ThreadsafeQueue(ThreadsafeQueue<T> &&other) {
		std::lock_guard < std::mutex > lock(mutex_);
		queue_ = std::move(other.queue_);
	}

	virtual ~ThreadsafeQueue() {
	}

	unsigned long size() const {
		std::lock_guard < std::mutex > lock(mutex_);
		return queue_.size();
	}

	std::optional<T> pop() {
		std::lock_guard < std::mutex > lock(mutex_);
		if (queue_.empty()) {
			return {};
		}
		T tmp = queue_.front();
		queue_.pop();
		return tmp;
	}

	void push(const T &item) {
		std::lock_guard < std::mutex > lock(mutex_);
		queue_.push(item);
	}
};

struct IOState {
	IOState() :
			pin(-1), value(-1) {
	}
	IOState(int pin, int value) :
			pin(pin), value(value) {
	}
	int pin;
	int value;
};

class Subscriber {
public:
	virtual void ioEvent(IOState state) = 0;
};

class PrintEvent : public Subscriber {
public:
	virtual void ioEvent(IOState state) {
		std::cout << "Pin " << state.pin << " = " << state.value << std::endl;
	}
};

class IOBoard {
public:
	IOBoard(int pi, int clock, int mosi, int miso, int ce0, int ce1, int latch,
			int reads_per_sec = 30) :
			pi(pi), clock(clock), mosi(mosi), miso(miso), ce0(ce0), ce1(ce1), latch(
					latch), reads_per_sec(reads_per_sec) {
		writeBuffer.resize(ioChannels, 0);
		readBuffer.resize(ioChannels + adcChannels, 0);
		lastReadBuffer = readBuffer;
		lastWriteBuffer = writeBuffer;

		delay = (int) (1000.0 * 1.0 / ((double) reads_per_sec));

		std::cout << "Read delay = " << delay << " ms" << std::endl;

		set_mode(pi, clock, PI_OUTPUT);
		set_mode(pi, mosi, PI_OUTPUT);
		set_mode(pi, miso, PI_INPUT);
		set_mode(pi, ce0, PI_OUTPUT);
		set_mode(pi, ce1, PI_OUTPUT);
		set_mode(pi, latch, PI_OUTPUT);

		for (int i = 0; i < adcChannels; i++) {
			adc_commands[i] = std::vector<int>(5, 0);
			adc_commands[i][0] = 1;
			adc_commands[i][1] = 1;
			adc_commands[i][2] = i & 1;
			adc_commands[i][3] = (i & 2) >> 1;
			adc_commands[i][4] = (i & 4) >> 2;
		}

	}

	IOBoard(int pi, int reads_per_sec = 1) :
			clock(21), mosi(20), miso(19), ce0(16), ce1(5), latch(26), pi(pi), reads_per_sec(
					reads_per_sec) {

		std::cout << "Constructor" << std::endl;

		writeBuffer.resize(ioChannels, 0);
		readBuffer.resize(ioChannels + adcChannels, 0);
		lastReadBuffer = readBuffer;
		lastWriteBuffer = writeBuffer;

		delay = (int) (1000.0 * 1.0 / ((double) reads_per_sec));

		std::cout << "Read delay = " << delay << " ms" << std::endl;

		set_mode(pi, clock, PI_OUTPUT);
		set_mode(pi, mosi, PI_OUTPUT);
		set_mode(pi, miso, PI_INPUT);
		set_mode(pi, ce0, PI_OUTPUT);
		set_mode(pi, ce1, PI_OUTPUT);
		set_mode(pi, latch, PI_OUTPUT);

		adc_commands.resize(adcChannels);
		for (int i = 0; i < adcChannels; i++) {
			adc_commands[i] = std::vector<int>(5, 0);
			adc_commands[i][0] = 1;
			adc_commands[i][1] = 1;
			adc_commands[i][2] = i & 1;
			adc_commands[i][3] = (i & 2) >> 1;
			adc_commands[i][4] = (i & 4) >> 2;
		}
	}

	void subscribe(int pin, std::shared_ptr< Subscriber > s) {
		if(subscribers.find(pin) == subscribers.end()) {
			subscribers[pin] = std::vector< std::shared_ptr< Subscriber > >();
		}
		subscribers[pin].push_back(s);
	}
	
	void unsubscribe(int pin, std::shared_ptr< Subscriber > s) {
		if(subscribers.find(pin) != subscribers.end()) {
			std::vector< std::shared_ptr< Subscriber > >::iterator found = std::find(subscribers[pin].begin(), subscribers[pin].end(), s);
			if(found != subscribers[pin].end()) {
				subscribers[pin].erase(found);
			}
		}
	}
	
	void clearSubscribers(int pin) {
		if(subscribers.find(pin) != subscribers.end()) {
			subscribers[pin].clear();
			subscribers.erase(pin);
		}
	}
	
protected:

	void processDigitalIO() {

		gpio_write(pi, latch, 0);
		gpio_write(pi, clock, 0);
		gpio_write(pi, ce0, 1);

		gpio_trigger(pi, clock, 1, 1);

		gpio_write(pi, ce0, 0);

		gpio_trigger(pi, clock, 1, 1);
		gpio_write(pi, ce0, 1);

		for (int i = 0; i < writeBuffer.size(); i++) {
			readBuffer[i] = gpio_read(pi, miso);
			//std::cout << "Read buffer[" << i << "] = " << readBuffer[i] << std::endl;
			gpio_write(pi, mosi, writeBuffer[i]);
			gpio_trigger(pi, clock, 1, 1);
		}

		gpio_write(pi, ce0, 1);
		gpio_write(pi, latch, 1);

		gpio_trigger(pi, clock, 1, 1);
	}

	int readADCChannel(int channel) {
		//std::cout << "Reading channel " << channel << std::endl;
		int ad = 0;
		//# 1. CS LOW.
		gpio_write(pi, ce1, 1);
		gpio_write(pi, ce1, 0);

		//# 2. Start clock
		gpio_write(pi, clock, 0);

		//# 3. Input MUX address
		std::vector<int> &cmd = adc_commands[channel];

		for (auto &&c : cmd) {		// i in cmd: # start bit + mux assignment
			if (c == 1)
				gpio_write(pi, mosi, 1);
			else
				gpio_write(pi, mosi, 0);
			gpio_trigger(pi, clock, 1, 1);
		}

		//# 4. read 8 ADC bits
		ad = 0;
		for (int i = 0; i < 8; i++) {		// range(8):
			gpio_trigger(pi, clock, 1, 1);
			ad <<= 1;		// # shift bit
			if (gpio_read(pi, miso) == 1)
				ad |= 0x1;		// # set first bit
		}

		//# 5. reset
		gpio_write(pi, ce1, 1);
		gpio_write(pi, mosi, 0);
		gpio_trigger(pi, clock, 1, 1);
		gpio_trigger(pi, clock, 1, 1);
		//std::cout << "Result = " << ad << std::endl;

		return ad;
	}

	void readADC() {
		//int sensitivity = 2;
		for (int i = 0; i < adcChannels; i++) {
			readBuffer[i + ioChannels] = readADCChannel(i);
			//std::cout << "Read buffer[" << (i + ioChannels) << "] = "
					//<< readBuffer[i + ioChannels] << std::endl;
		}
	}

public:

	std::thread spawn() {
		return std::thread(&IOBoard::loop, this);
	}
	
	void processEvents() {
		//go through outgoing messages and post to subscribers
		while( std::optional<IOState> msg = fromBoard.pop() ) {
			IOState state = *msg;
			if(subscribers.find(state.pin) != subscribers.end()) {
				std::vector< std::shared_ptr< Subscriber > >& listeners = subscribers[state.pin];
				for(auto && listener : listeners) {
					listener->ioEvent(state);
				}
			}
		}
	}
	
	void set(int pin, int value) {
		IOState state(pin, value);
		toBoard.push(state);
	}

protected:

	void loop() {

		//int bit = 0;

		while (true) {

			writeBuffer = lastWriteBuffer;

			//writeBuffer[bit] = !writeBuffer[bit];

			//bit++;

			//if (bit >= writeBuffer.size())
			//	bit = 0;

			//std::cout << "Setting up write buffer" << std::endl;
			while (std::optional<IOState> toSend = toBoard.pop()) {

				//treat any negative value as term signal
				if (toSend->value < 0) {
					std::cout << "Received term signal" << std::endl << std::flush;
					return;
				}

				writeBuffer[toSend->pin] = toSend->value;

			}

			lastWriteBuffer = writeBuffer;

			//readBuffer.assign(0, readBuffer.size());

			//std::cout << "Shift register IO" << std::endl;
			processDigitalIO();

			//std::cout << "Read ADC" << std::endl;
			readADC();

			//std::cout << "Post events" << std::endl;
			
			for(int i = 0; i < readBuffer.size(); i++) {
				if(readBuffer[i] != lastReadBuffer[i]) {
					IOState state(i, readBuffer[i]);
					fromBoard.push(state);
					//std::cout << "Pin " << state.pin << " = " << state.value << std::endl;
				}
			}

			lastReadBuffer = readBuffer;
			
			//std::cout << "Sleep" << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		}
	}

public:
	static const int ioChannels = 16;
	static const int adcChannels = 8;
	
protected:
	int pi;
	int clock;
	int mosi;
	int miso;
	int ce0;
	int ce1;
	int latch;
	int reads_per_sec;

	int delay;

	std::vector<int> writeBuffer;
	std::vector<int> readBuffer;
	std::vector<int> lastWriteBuffer;
	std::vector<int> lastReadBuffer;

	std::vector<std::vector<int> > adc_commands;

	ThreadsafeQueue<IOState> toBoard;
	ThreadsafeQueue<IOState> fromBoard;
	
	std::map<int, std::vector< std::shared_ptr< Subscriber > > > subscribers;

};

class App : public PrintEvent {
public:
	App(IOBoard& board, int freq) : board(board) {
		delay = (int) (1000.0 / (float) freq);
	}
	virtual ~App() {}
	
public:
	std::thread startMainLoop() {
		return std::thread(&App::mainLoop, this);
	}
	
	virtual void ioEvent(IOState state) {
		PrintEvent::ioEvent(state);
		board.set(state.pin, state.value);
	}
		
protected:

	void init() {
		for(int i = 0; i < board.ioChannels; i++) {
			board.set(i, 0);
		}
	}

	void mainLoop() {
		
		init();
		
		while(true) {
			board.processEvents();
			
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
		}
	}
	
private:
	IOBoard& board;
	int delay;
};

/*
int main(int argc, char **argv) {

	std::cout << "Hello world!" << std::endl;

	int pi = pigpio_start(0, 0);
	
		//IOBoard board(30);
	std::cout << "GPIO Init" << std::endl;
	IOBoard board(pi, 30);
	
	std::shared_ptr< App > app = std::make_shared<App>(board, 60);
	
	for(int i = 0; i < 16; i++) {
		board.subscribe(i, app);
	}
	
	std::thread t = board.spawn();//(&IOBoard::loop, board);//IOBoard(pi, 1));
	
	std::cout << "Board thread started" << std::endl;
	
	std::thread a = app->startMainLoop();
	
	std::cout << "App thread started" << std::endl;
	
	t.join();
	a.join();
	
	std::cout << "Stopping pigpio" << std::endl;
	
	pigpio_stop(pi);

	return 0;
}
*/


