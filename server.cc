#include <omnetpp.h>
//using namespace omnetpp; //for Omnet++ ver. 5

class Server : public cSimpleModule
{		
  private:
    cQueue queue;	            //the queue of jobs; it is assumed that the first job in the queue is the one being serviced
	cMessage *departure;        //special message; it reminds about the end of service and the need for job departure
	simtime_t departure_time;   //time of the next departure
	int N; //buffer size
	cDoubleHistogram buffer_overflow_period;
	float L; //loss ratio
	int rejected_packets;
	int all_packets;

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msgin);

};

Define_Module(Server);


void Server::initialize()
{	
	departure = new cMessage("Departure");
	N = par("buffer_size");
	L = 0.0;
	WATCH(L);
	buffer_overflow_period.setName("buffer overflow period");
	//buffer_overflow_period.setRange(service_time_min, service_time_min);

}


void Server::handleMessage(cMessage *msgin)  //two types of messages may arrive: a job from the source, or the special message initiating job departure
{		
    if (msgin==departure)   //job departure
	{
		cMessage *msg = (cMessage *)queue.pop();    //remove the finished job from the head of the queue
		//if(queue.length() == N - 1){
		    //overflow_period.collect(simTime()-start)
		//}


		send(msg,"out");                            //depart the finished job
		if (!queue.isEmpty())                         //if the queue is not empty, initiate the next service, i.e. schedule the next departure event in the future
		{
			departure_time=simTime()+par("service_time");
	        scheduleAt(departure_time,departure);
		}
	}
	else                 //job arrival
	{				
	    ++all_packets;
	    if(queue.length() >= N){
	        delete msgin;
	        ++rejected_packets;
	        L = (float)rejected_packets/(float)all_packets;
	        return;
	    }
	    else if (queue.isEmpty())  //if the queue is empty, the job that has just arrived has to be served immediately, i.e. the departure event of this job has to be scheduled in the future
		{
			departure_time=simTime()+par("service_time");
            scheduleAt(departure_time,departure);
		}
		queue.insert(msgin); //insert the job at the end of the queue
		L = (float)rejected_packets/(float)all_packets;
	}



}

