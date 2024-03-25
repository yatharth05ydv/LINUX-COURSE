#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#define SIZE 100
#define NOS 15
struct queue
{
    char messages[NOS][SIZE];
    int front;
    int rear;
};

sem_t *mutex;
sem_t *wr;

void enqueue(struct queue *mq, char msg[]) 
{
	if((mq->rear + 1)%NOS == mq->front )
    {
    	printf("QUEUE OVERFLOW");
        exit(0);
    }
    sem_wait(wr); 
    strcpy(mq->messages[mq->rear], msg);
    mq->rear = (mq->rear + 1) % NOS;
    sem_post(wr); 
}

void dequeue(struct queue *mq, char msg[]) 
{
	if(mq->rear == mq->front)
    {
	   	printf("QUEUE UNDERFLOW");
        exit(0);
    }
    sem_wait(mutex);
    sem_wait(wr); 
    strcpy(msg, mq->messages[mq->front]);
    strcpy(mq->messages[mq->front],"");
    mq->front = (mq->front + 1) % NOS;
    sem_post(wr); 
    sem_post(mutex); 
}

void display(struct queue *mq) 
{
    sem_wait(mutex); 
    int i = mq->front;
    printf("Messages in Queue are:\n");
    while (i != mq->rear) 
    {
        printf("%s\n", mq->messages[i]);
        i = (i + 1) % NOS;
    }
    sem_post(mutex); 
}

int main() 
{
    int shm_fd;
    struct queue *mq;
    char buff[SIZE];

    mutex = sem_open("/mutex", O_CREAT, 0666, 1);
    wr = sem_open("/wr", O_CREAT, 0666, 1);

    shm_fd = shm_open("/message_queue", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    ftruncate(shm_fd, sizeof(struct queue));

    mq = mmap(NULL, sizeof(struct queue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (mq == MAP_FAILED) 
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    mq->front = 0;
    mq->rear = 0;

    printf("MESSAGE QUEUE\n\n");

    pid_t pid = fork();
    if (pid == 0) 
    {
        while (1) 
        {
            printf("\n1:Insert\n2:Delete\n3:Display\n4:exit ");
            printf("\nEnter the Choice: ");
            int choice;
            scanf("%d", &choice);

            switch (choice) 
            {
                case 1:
                    printf("Enter the Message to be sent: ");
                    scanf("%s", buff);
                    enqueue(mq, buff);
                    break;
                case 2:
                    dequeue(mq, buff);
                    printf("Removed message is %s\n", buff);
                    break;
                case 3:
                    display(mq);
                    break;
                case 4:
                    sem_close(mutex);
                    sem_close(wr);
                    munmap(mq, sizeof(struct queue));
                    shm_unlink("/message_queue");
                    sem_unlink("/mutex");
                    sem_unlink("/wr");
                    exit(0);
                default:
                    printf("Invalid choice\n");
                    exit(1);
            }
        }
    } 
    else if (pid > 0) 
    {
        wait(NULL);
    } 
    else 
    {
        printf("Fork failed\n");
        return 1;
    }

    return 0;
}

