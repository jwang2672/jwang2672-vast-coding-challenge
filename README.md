# jwang2672-vast-coding-challenge
Vast take-home coding challenge

# Outline

### Key Assumptions & Simplifications
* Travel time between site and station is 30 minutes in both directions.
* Mining time is uniformly random between 1 and 5 hours (60–300 minutes).
* Unloading time is always 5 minutes.
* We track events in minutes.
* We assume that at t=0t=0, all trucks start empty at the mining site and immediately begin mining for a random duration.

### Core Classes
* Truck: Holds truck-specific data (e.g., ID, stats, state).
* Station: Holds station-specific data (e.g., ID, queue of trucks).
* Event: Encapsulates an event (time, type, truck, station, etc.).
* Simulation: Manages the global event queue, time advancement, and statistics aggregation.

### Event Types
* FINISH_MINING: Truck finishes mining and is ready to travel to station.
* ARRIVE_STATION: Truck arrives at a station.
* START_UNLOADING: Truck begins unloading (only when a station becomes free).
* FINISH_UNLOADING: Truck finishes unloading and will travel back to site.

### Simulation Flow (Discrete Event)
* Initialize trucks, stations, event queue.
* Schedule initial FINISH_MINING events (one per truck) based on random mining time.
* Process events in chronological order until simulation time exceeds 72 hours (4320 minutes).
* Depending on the event type, create new events (e.g., once unloading is done, schedule the next mining completion).

### Statistics
* Each truck tracks number of loads delivered, total wait time, total travel time, etc.
* Each station tracks utilization time (how often it was busy).

### Test Cases
* We include a simple main() with sample test runs.

### Future Improvements
* Redesign findBestStation() and Station class such that it has a var futureTimeFree to determine shortest queue
* The current method of smallest sized queue is inaccurate to 5 minutes
* Use a MinHeap instead of linear search to go from O(N) to O(NlogN) time complexity