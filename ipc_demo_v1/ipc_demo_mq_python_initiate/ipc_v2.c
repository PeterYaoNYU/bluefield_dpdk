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
send_to_ml_model(uint64_t* ts, mqd_t mq_desc, size_t input_size)
{
	printf("sending to the descriptor: %d\n", (int)mq_desc);
	int ret = mq_send(mq_desc, (const char*)ts, input_size, 0);

	for (int i = 0; i < ARR_SIZE; i++){
		printf("%ld\n",ts[i]);
	}

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

	uint64_t * arr_timestamp = (uint64_t *)malloc(sizeof(uint64_t) * ARR_SIZE);

	for (int i = 10; i < ARR_SIZE + 10; i++){
		arr_timestamp[i-10]= i * i;
		printf("%ld\n",arr_timestamp[i-10]);
	}

    mq = mq_open(QUEUE_NAME, O_RDWR);
	size_t size = sizeof(uint64_t) * ARR_SIZE;
	printf("size of the msg: %ld\n", size);

	send_to_ml_model(arr_timestamp, mq, size);
	printf("send finished \n");

	sleep(1000);

	mq_close(mq);
	mq_unlink(QUEUE_NAME);
	printf("Message queue closed.\n");
}