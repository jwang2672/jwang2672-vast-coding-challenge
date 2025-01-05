#include <iostream>
#include <queue>
#include <vector>
#include <random>
#include <chrono>
#include <functional>
#include <memory>
#include <iomanip>
#include <algorithm>

/*
 * ================================
 * CONFIGURATION CONSTANTS
 * ================================
 */
static const int MINING_TIME_MIN = 60;   // 1 hour  in minutes
static const int MINING_TIME_MAX = 300;  // 5 hours in minutes
static const int TRAVEL_TIME = 30;       // 30 minutes (site <-> station)
static const int UNLOAD_TIME = 5;        // 5 minutes
static const int SIMULATION_TIME = 4320; // 72 hours in minutes (72 * 60)

/*
 * ================================
 * ENUM: EventType
 * ================================
 * Represents the types of events we handle in the simulation.
 */
enum class EventType
{
    FINISH_MINING,   // Truck finishes mining at the site
    ARRIVE_STATION,  // Truck arrives at an unload station
    START_UNLOADING, // Truck starts unloading
    FINISH_UNLOADING // Truck finishes unloading
};

/*
 * ================================
 * CLASS: Truck
 * ================================
 * Represents a mining truck and tracks various statistics.
 */
class Truck
{
public:
    int id;
    int loadsDelivered;      // how many loads the truck has delivered
    double arrivalEventTime; // when turck arrived at station (used to calculate wait)

    double totalWaitTime;   // total time spent waiting in queue
    double totalTravelTime; // total time spent traveling
    double totalMiningTime; // total time spent mining
    double totalUnloadTime; // total time spent unloading

    // Constructor
    Truck(int _id)
        : id(_id), loadsDelivered(0), arrivalEventTime(0.0), totalWaitTime(0.0),
          totalTravelTime(0.0), totalMiningTime(0.0), totalUnloadTime(0.0)
    {
    }

    // For debugging/logging
    void printStats() const
    {
        std::cout << "Truck " << id << " Statistics:\n"
                  << "  Loads Delivered: " << loadsDelivered << "\n"
                  << "  Total Wait Time (min): " << totalWaitTime << "\n"
                  << "  Total Travel Time (min): " << totalTravelTime << "\n"
                  << "  Total Mining Time (min): " << totalMiningTime << "\n"
                  << "  Total Unload Time (min): " << totalUnloadTime << "\n"
                  << std::endl;
    }
};

/*
 * ================================
 * CLASS: Station
 * ================================
 * Represents an unload station where one truck can unload at a time.
 */
class Station
{
public:
    int id;
    bool isBusy;
    double busyUntil;     // track until what time the station is busy
    double totalBusyTime; // how long the station was busy (used for utilization calculation)

    // Queue of trucks waiting for this station
    std::queue<int> truckQueue; // store truck IDs in queue

    // Constructor
    Station(int _id) : id(_id), isBusy(false), busyUntil(0.0), totalBusyTime(0.0) {}

    // For debugging/logging
    void printStats() const
    {
        std::cout << "Station " << id << " Statistics:\n"
                  << "  Total Busy Time (min): " << totalBusyTime << "\n"
                  << std::endl;
    }

    // We need to order station based on shortest truckQueue
    // bool operator>(const Station &other) const
    // {
    //     return this->truckQueue.size() > other.truckQueue.size();
    // }
};

/*
 * ================================
 * STRUCT: Event
 * ================================
 * Represents a single simulation event with a time, type, and associated IDs.
 */
struct Event
{
    double time;    // time in the simulation (minutes)
    EventType type; // event type
    int truckId;    // which truck is involved
    int stationId;  // which station is involved, if applicable

    // We need to order events in a priority queue by earliest time
    bool operator>(const Event &other) const
    {
        return this->time > other.time;
    }
};

/*
 * ================================
 * CLASS: Simulation
 * ================================
 * Manages the overall simulation, event queue, and data structures.
 */
class Simulation
{
private:
    // Priority queue of events, earliest event first
    std::priority_queue<Event, std::vector<Event>, std::greater<Event>> eventQueue;

    // Priority queue of stations, earliest station first for trucks
    // to implement minHeap for station with smallest queue

    // The trucks and stations
    std::vector<Truck> trucks;
    std::vector<Station> stations;

    // Random engine for mining durations
    std::mt19937 rng;
    std::uniform_int_distribution<int> miningDist;

    // Current time in simulation
    double currentTime;

public:
    Simulation(int numTrucks, int numStations)
        : rng(std::random_device{}()), miningDist(MINING_TIME_MIN, MINING_TIME_MAX), currentTime(0.0)
    {
        // Initialize trucks
        for (int i = 0; i < numTrucks; ++i)
        {
            trucks.push_back(Truck(i));
        }
        // Initialize stations
        for (int i = 0; i < numStations; ++i)
        {
            stations.push_back(Station(i));
        }
    }

    /*
     * Runs the simulation up to SIMULATION_TIME minutes.
     */
    void run()
    {
        // Schedule initial FINISH_MINING events for each truck
        for (auto &truck : trucks)
        {
            int miningTime = miningDist(rng);
            scheduleEvent(currentTime + miningTime, EventType::FINISH_MINING, truck.id, -1);
        }

        // Process events until we exceed SIMULATION_TIME
        while (!eventQueue.empty())
        {
            Event evt = eventQueue.top();
            eventQueue.pop();

            // If the event is beyond our simulation window, we stop processing
            if (evt.time > SIMULATION_TIME)
            {
                break;
            }

            // Advance currentTime
            currentTime = evt.time;

            // Handle event
            handleEvent(evt);
        }
    }

    /*
     * Prints statistics for all trucks and stations.
     */
    void printStats()
    {
        std::cout << "\n==================== Simulation Statistics ====================\n";
        // Print Truck Stats
        for (const auto &truck : trucks)
        {
            truck.printStats();
        }
        // Print Station Stats
        for (auto &station : stations)
        {
            // If the station was busy until a certain time, we add that to totalBusyTime
            // in case the station is still busy at the simulation end.
            if (station.isBusy && station.busyUntil < SIMULATION_TIME)
            {
                station.totalBusyTime += (station.busyUntil - currentTime) < 0 ? 0 : (SIMULATION_TIME - currentTime);
            }
            station.printStats();
            double utilization = (station.totalBusyTime / SIMULATION_TIME) * 100.0;
            std::cout << "  Utilization: " << utilization << " %\n"
                      << std::endl;
        }

        std::cout << "\n===============================================================\n\n\n";
    }

private:
    /*
     * Schedule a new event by pushing it into the priority queue.
     */
    void scheduleEvent(double time, EventType type, int truckId, int stationId)
    {
        Event evt{time, type, truckId, stationId};
        eventQueue.push(evt);
    }

    /*
     * Handle the given event based on its type.
     */
    void handleEvent(const Event &evt)
    {
        switch (evt.type)
        {
        case EventType::FINISH_MINING:
            onFinishMining(evt.truckId);
            break;
        case EventType::ARRIVE_STATION:
            onArriveStation(evt.truckId);
            break;
        case EventType::START_UNLOADING:
            onStartUnloading(evt.truckId, evt.stationId);
            break;
        case EventType::FINISH_UNLOADING:
            onFinishUnloading(evt.truckId, evt.stationId);
            break;
        default:
            break;
        }
    }

    /*
     * A truck finishes mining at the site -> travel to station
     */
    void onFinishMining(int truckId)
    {
        trucks[truckId].totalTravelTime += TRAVEL_TIME;
        scheduleEvent(currentTime + TRAVEL_TIME, EventType::ARRIVE_STATION, truckId, -1);
    }

    /*
     * A truck arrives at the station -> find the station with the shortest queue
     * or an available station, and queue up.
     */
    void onArriveStation(int truckId)
    {
        // If there are 0 stations, Truck waits forever
        if (stations.size() <= 0)
        {
            trucks[truckId].totalWaitTime += SIMULATION_TIME - currentTime;
            return;
        }

        // Find the station with the minimal queue time or an available station
        int chosenStationId = findBestStation();

        // record time truck arrives at station
        trucks[truckId].arrivalEventTime = currentTime;

        // Queue the truck at that station
        stations[chosenStationId].truckQueue.push(truckId);

        // If the station is not busy, the truck can start unloading immediately
        if (!stations[chosenStationId].isBusy)
        {
            scheduleEvent(currentTime, EventType::START_UNLOADING,
                          stations[chosenStationId].truckQueue.front(),
                          chosenStationId);
        }
    }

    /*
     * The chosen station starts unloading the front truck in its queue.
     */
    void onStartUnloading(int truckId, int stationId)
    {
        Station &station = stations[stationId];

        // Mark station as busy
        station.isBusy = true;

        // Calculate how long the truck has been waiting
        trucks[truckId].totalWaitTime += currentTime - trucks[truckId].arrivalEventTime;

        // Truck starts unloading, schedule FINISH_UNLOADING
        trucks[truckId].totalUnloadTime += UNLOAD_TIME;
        double finishTime = currentTime + UNLOAD_TIME;

        // Station will be busy until finishTime
        station.busyUntil = finishTime;

        // For this simple simulation, UNLOAD_TIME is added to totalBusyTime
        station.totalBusyTime += (finishTime - currentTime); // station is busy for this duration

        scheduleEvent(finishTime, EventType::FINISH_UNLOADING, truckId, stationId);
    }

    /*
     * The truck finishes unloading -> increment loads delivered; then travel back to mine site.
     */
    void onFinishUnloading(int truckId, int stationId)
    {
        Station &station = stations[stationId];

        // One load delivered
        trucks[truckId].loadsDelivered++;

        // Remove truck from station queue
        if (!station.truckQueue.empty())
        {
            station.truckQueue.pop();
        }

        // If there's another truck in queue, schedule START_UNLOADING for that truck
        if (!station.truckQueue.empty())
        {
            // The next truck can start unloading immediately at currentTime
            scheduleEvent(currentTime, EventType::START_UNLOADING,
                          station.truckQueue.front(), stationId);
        }
        else
        {
            // Mark station as not busy
            station.isBusy = false;
        }

        // Truck travels back to site to mine again
        trucks[truckId].totalTravelTime += TRAVEL_TIME;
        double arrivalAtMineTime = currentTime + TRAVEL_TIME;

        // After traveling back, it starts mining again for random duration
        int nextMiningTime = miningDist(rng);
        trucks[truckId].totalMiningTime += nextMiningTime;
        scheduleEvent(arrivalAtMineTime + nextMiningTime, EventType::FINISH_MINING, truckId, -1);
    }

    /*
     * Finds the station with the shortest queue (or an available one).
     * If multiple stations have the same queue size, pick one arbitrarily.
     * (if more time rewrite to use minHeap to go from O(N) to log(N)
     * this function may not find smallest queue times if queue sizes are same
     * need to add a public variable that stores future complete time)
     */
    int findBestStation()
    {
        int bestStationId = -1;
        size_t minQueueSize = std::numeric_limits<size_t>::max();

        for (auto &station : stations)
        {
            size_t queueSize = station.truckQueue.size();
            if (queueSize < minQueueSize)
            {
                minQueueSize = queueSize;
                bestStationId = station.id;
            }
        }
        return bestStationId;
    }
};

/*
 * ================================
 * MAIN: Test Cases
 * ================================
 *
 * if more time, would add test cases to each function
 *
 * using debugger to manually verifiy functionality
 */
int main()
{
    // test class 0: General tests
    //  Test 0.1: 3 trucks, 1 station
    {
        std::cout << "==== Test Case 0.1: 3 Trucks, 1 Station ====\n";
        Simulation sim(3, 1);
        sim.run();
        sim.printStats();
    }

    // Test 0.2: 5 trucks, 2 stations
    {
        std::cout << "==== Test Case 0.2: 5 Trucks, 2 Stations ====\n";
        Simulation sim(5, 2);
        sim.run();
        sim.printStats();
    }

    // Test 0.3: 10 trucks, 3 stations
    {
        std::cout << "==== Test Case 0.3: 10 Trucks, 3 Stations ====\n";
        Simulation sim(10, 3);
        sim.run();
        sim.printStats();
    }

    // Test 0.3: 100 trucks, 3 stations
    // test that it chooses smallest station (use debugger)
    {
        std::cout << "==== Test Case 0.3: 10 Trucks, 3 Stations ====\n";
        Simulation sim(50, 3);
        sim.run();
        sim.printStats();
    }

    // Test class 1: weird test cases
    // Test 1.1: no waits
    {
        std::cout << "==== Test Case 1.1: 1 Trucks, 1 Stations ====\n";
        Simulation sim(1, 1);
        sim.run();
        sim.printStats();
    }

    // Test 1.2: lots of waits
    {
        std::cout << "==== Test Case 1.2: 30 Trucks, 1 Stations ====\n";
        Simulation sim(30, 1);
        sim.run();
        sim.printStats();
    }

    // Test 2: 0 stations or trucks or both
    // Test 2.1`
    {
        std::cout << "==== Test Case 2.1: 0 Trucks, 1 Stations ====\n";
        Simulation sim(0, 1);
        sim.run();
        sim.printStats();
    }

    // test 2.2
    {
        std::cout << "==== Test Case 2.2: 1 Trucks, 0 Stations ====\n";
        Simulation sim(1, 0);
        sim.run();
        sim.printStats();
    }

    // test 2.3
    {
        std::cout << "==== Test Case 2.3: 0 Trucks, 0 Stations ====\n";
        Simulation sim(0, 0);
        sim.run();
        sim.printStats();
    }
    return 0;
}
