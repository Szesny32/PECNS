#include <omnetpp.h>
using namespace omnetpp; //for Omnet++ ver. 5

class Server : public cSimpleModule
{
  private:
    cQueue queue;               //the queue of jobs; it is assumed that the first job in the queue is the one being serviced
    int N;                      //limitation for the maximum queue size
    cMessage *departure;        //special message; it reminds about the end of service and the need for job departure
    simtime_t departure_time;   //time of the next departure
    float L;                    //loss ratio
    int packets;
    int rejected_packets;
    cDoubleHistogram time_of_buffer_overflow_period;                //is the time from the moment when the buffer gets full to the first departure moment after that
    cLongHistogram consecutive_losses_distribution;         //namely, a series of consecutive losses consists of all packets/jobs lost in one overflow period.
    cDoubleHistogram time_to_buffer_overflow;

    simtime_t overflow_timpestamp;
    simtime_t empty_queue_timpestamp;
    bool timer_is_running;

    int period_lost_packets;        //number of losses consists of all packets/jobs lost in one overflow period
    float B;                        //burst ratio - describes the statistical structure of packet losses
    float G;                        //average length of series of consecutive losses
    float K;                        //average length of this series expected for purely randomlosses

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msgin);

};

Define_Module(Server);


void Server::initialize()
{
    departure = new cMessage("Departure");
    time_of_buffer_overflow_period.setName("Buffer overflow period");
    time_of_buffer_overflow_period.setNumCells(200);
    time_of_buffer_overflow_period.setRange(0, 2.2);
    consecutive_losses_distribution.setName("Distribution of the number of consecutive losses");
    consecutive_losses_distribution.setNumCells(200);
    time_to_buffer_overflow.setName("Time to buffer overflow");
    time_to_buffer_overflow.setNumCells(200);


    N = par("buffer_size");
    packets = 0;
    period_lost_packets = 0;
    rejected_packets = 0;
    L = (float)rejected_packets / (float)packets;
    WATCH(L);
    overflow_timpestamp = 0;

    G = consecutive_losses_distribution.getMean();
    K = 1.0 / (1.0 - L);
    B = G / K;
    WATCH(B);

    empty_queue_timpestamp = 0;
    timer_is_running = true;
}


void Server::handleMessage(cMessage *msgin)  //two types of messages may arrive: a job from the source, or the special message initiating job departure
{
    if (msgin==departure)   //job departure
    {
        cMessage *msg = (cMessage *)queue.pop();    //remove the finished job from the head of the queue
        send(msg,"out");                            //depart the finished job
        if(queue.length() == N-1){
            time_of_buffer_overflow_period.collect(simTime()-overflow_timpestamp);
            if(period_lost_packets){            //DO NOT COLLECT "0-loss strike" - it affects the average
                consecutive_losses_distribution.collect(period_lost_packets);
            }
            overflow_timpestamp = 0;
            period_lost_packets = 0;
        }

        if (!queue.isEmpty())                         //if the queue is not empty, initiate the next service, i.e. schedule the next departure event in the future
        {
            departure_time=simTime()+par("service_time");
            scheduleAt(departure_time,departure);
        } else {
                if(timer_is_running == false){
                    EV<<"queue.isEmpty()";
                    empty_queue_timpestamp = simTime();
                    timer_is_running = true;
                }
        }

    }
    else                    //job arrival
    {
        packets++;
        if(queue.length() >= N){
           delete msgin;
           rejected_packets++;
           period_lost_packets++;
           L = (float)rejected_packets / (float)packets;
           G = consecutive_losses_distribution.getMean();
           K = 1.0 / (1.0 - L);
           B = G / K;
           return;
        }
        else if (queue.isEmpty())  //if the queue is empty, the job that has just arrived has to be served immediately, i.e. the departure event of this job has to be scheduled in the future
        {
            departure_time=simTime()+par("service_time");
            scheduleAt(departure_time,departure);

        }
        L = (float)rejected_packets / (float)packets;
        G = consecutive_losses_distribution.getMean();
        K = 1.0 / (1.0 - L);
        B = G / K;

        queue.insert(msgin); //insert the job at the end of the queue
        if(queue.length() == N){
            overflow_timpestamp = simTime();
            period_lost_packets = 0;
            if(timer_is_running){
                time_to_buffer_overflow.collect(simTime()-empty_queue_timpestamp);
                timer_is_running = false;
            }

            EV<<" time_to_buffer_overflow.collect "<<simTime()-empty_queue_timpestamp;

        }

    }
}
