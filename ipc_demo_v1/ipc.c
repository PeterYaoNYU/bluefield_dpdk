#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>


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
send_to_ml_model(char * mbuf, mqd_t mq_desc, size_t input_size)
{
	printf("sending to the descriptor: %d\n", (int)mq_desc);
	int ret = mq_send(mq_desc, mbuf, input_size, 0);

	if (ret == -1){
		perror("mq_send");
		fprintf(stderr, "Error: %s\n", strerror(errno));
		exit(1);
	}
	return 0;
}

int main(int argc, char * argv[])
{
    mq = create_send_msg_queue();
	char * mbuf = "thank you";
	size_t size = sizeof(mbuf);

	send_to_ml_model(mbuf, mq, size);
	printf("send finished \n");
}