#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

using namespace std;

class EventData {
public:
    int id;
    EventData(int i) : id(i) {}
};

class Monitor {
private:
    mutex mtx;
    condition_variable cv;
    EventData* shared_data = nullptr;
    bool data_is_ready = false;

public:
    void push(EventData* newData) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return !data_is_ready; });

        shared_data = newData;
        data_is_ready = true;

        cout << "[Поставщик] -> Событие отправлено (ID: " << newData->id << ")" << endl;

        cv.notify_one();
    }

    EventData* pop() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this] { return data_is_ready; });

        EventData* temp = shared_data;
        shared_data = nullptr;
        data_is_ready = false;

        cout << "[Потребитель] <- Событие получено (ID: " << temp->id << ")" << endl;

        cv.notify_one();

        return temp;
    }
};

Monitor monitor;

void producerThread() {
    int counter = 0;
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));
        EventData* data = new EventData(++counter);
        monitor.push(data);
    }
}

void consumerThread() {
    while (true) {
        EventData* data = monitor.pop();
        delete data;
    }
}

int main() {
    setlocale(LC_ALL, "");

    thread producer(producerThread);
    thread consumer(consumerThread);

    producer.join();
    consumer.join();

    return 0;
}