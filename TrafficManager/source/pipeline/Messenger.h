#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>

namespace traffic_manager {

    template<typename Data>
    struct DataPacket {
        int id;
        Data data;
    };

    template<typename Data>
    class Messenger {

    private:
        std::atomic<int> state_counter;
        Data data;

        std::mutex data_modification_mutex;
        std::condition_variable send_condition;
        std::condition_variable receive_condition;

    public:

        Messenger() {
            state_counter = 0;
        }
        ~Messenger() {}

        int SendData(DataPacket<Data> packet) {

            std::unique_lock<std::mutex> lock(data_modification_mutex);
            if (state_counter.load() == packet.id) {
                send_condition.wait(lock, [=] {return state_counter.load() != packet.id;});
            }
            data = packet.data;
            state_counter.store(state_counter.load() +1);
            int present_state = state_counter.load();
            receive_condition.notify_one();

            return present_state;
        }

        DataPacket<Data> ReceiveData(int old_state) {

            std::unique_lock<std::mutex> lock(data_modification_mutex);
            if (state_counter.load() == old_state) {
                receive_condition.wait(lock, [=] {return state_counter.load() != old_state;});
            }
            state_counter.store(state_counter.load() +1);
            DataPacket<Data> packet = {state_counter.load(), data};
            send_condition.notify_one();

            return packet;
        }

        int GetState() {
            return state_counter.load();
        }

    };
}
