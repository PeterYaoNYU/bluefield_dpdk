#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <inttypes.h>
#include <stddef.h>


#define QUEUE_NAME "/ml_data"

#define HEARTBEAT_N 10
#define DELTA_I 100
#define ARR_SIZE HEARTBEAT_N

struct __attribute__((packed)) hb_timestamp {
    uint64_t heartbeat_id;
    uint64_t hb_timestamp;
};

struct __attribute__((packed)) fd_info {
    // heartbeat interval in melli-second
    int delta_i;
    // estimated arrival time for the next heartbeat
    uint64_t ea;
    // // keep track of the last n packets
    // uint16_t n;

    // keep track of the last n timestamps
    // +2 to make the update procedure smoother
    struct hb_timestamp arr_timestamp[ARR_SIZE];

    // the timestamp that got evicted
    uint64_t evicted_time;

    int next_evicted;
    int next_avail;
};

struct hb_timestamp arr_timestamp[ARR_SIZE];

mqd_t mq;

void keyInteruptHandler(int signal) {
    if (signal == SIGINT) {
        // Close the message queue
        mq_close(mq);
		mq_unlink(QUEUE_NAME);
        printf("Message queue closed.\n");
        exit(0);
    }
}

int create_send_msg_queue()
{
    // create the memory queue in the system
	// if succeed, return the message queue descriptor for use of other functions
	// else, indicate that there is an error and quit the system
	struct mq_attr attr;

	attr.mq_flags = 0;
	attr.mq_maxmsg = 10;
	attr.mq_msgsize = 256;
	attr.mq_curmsgs = 0;

	// create the message queue explicitly
	mq = mq_open(QUEUE_NAME, O_CREAT | O_RDWR, 0666, &attr);
	printf("the msg queue desc is %d\n", (int)mq);
	if (mq == (mqd_t)-1) {
        perror("fail to create the message queue successfully");
        exit(1);
    } else {
		return mq;
	}
}

int 
send_to_ml_model(struct hb_timestamp* ts, mqd_t mq_desc, size_t input_size)
{
	printf("sending to the descriptor: %d\n", (int)mq_desc);
	int ret = mq_send(mq_desc, (const char*)ts, input_size, 0);

	if (ret == -1){
		perror("mq_send");
		exit(1);
	}
	return 0;
}

int main(int argc, char * argv[])
{
	// Install signal handler for SIGINT (keyboard interrupt)
    signal(SIGINT, keyInteruptHandler);

	for (int i = 10; i < ARR_SIZE + 10; i++){
		arr_timestamp[i].heartbeat_id = i;
		arr_timestamp[i].hb_timestamp = i * i;
		printf("%ld, %ld\n",arr_timestamp[i].heartbeat_id, arr_timestamp[i].hb_timestamp);
	}

    mq = create_send_msg_queue();
	size_t size = sizeof(arr_timestamp);
	printf("size of the msg: %ld\n", size);

	send_to_ml_model(arr_timestamp, mq, size);
	printf("send finished \n");

	sleep(1000);

	mq_close(mq);
	mq_unlink(QUEUE_NAME);
	printf("Message queue closed.\n");
}