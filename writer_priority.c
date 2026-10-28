/* ============================================================
 *  读者-写者问题（写者优先，避免写者饥饿）
 *  编译: gcc writer_priority.c -o writer_priority -lpthread
 *  运行: ./writer_priority
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define READER_NUM  4      /* 读者线程数 */
#define WRITER_NUM  2      /* 写者线程数 */

/* ---------- 信号量定义 ---------- */
sem_t r;        /* 互斥访问共享资源（数据库/文件） */
sem_t w;        /* 入口闸门：写者优先的关键 */
sem_t rmutex;   /* 保护 readcount 的互斥量 */
sem_t wmutex;   /* 保护 writecount 的互斥量 */

/* ---------- 共享变量 ---------- */
int readcount  = 0;    /* 正在阅读的读者数量 */
int writecount = 0;    /* 等待或正在写的写者数量 */
int shared_data = 0;   /* 被读写的数据 */

/* ---------------- 读者线程 ---------------- */
void *reader(void *arg)
{
    int id = *(int *)arg;

    /* 1. 先在入口 w 处排队（写者到来时新读者会被挡在这里） */
    sem_wait(&w);

    /* 2. 登记：第一个读者锁住资源，阻止写者进入 */
    sem_wait(&rmutex);
    readcount++;
    if (readcount == 1)
        sem_wait(&r);
    printf("[读者%d] 进入，当前读者数 = %d\n", id, readcount);
    sem_post(&rmutex);

    /* 3. 放开入口，允许排队中的其他线程继续 */
    sem_post(&w);

    /* 4. 临界区：读取共享数据 */
    printf("[读者%d] 正在读取数据 = %d\n", id, shared_data);
    usleep(200000);   /* 模拟读取耗时 0.2s */

    /* 5. 注销：最后一个读者释放资源 */
    sem_wait(&rmutex);
    readcount--;
    if (readcount == 0)
        sem_post(&r);
    printf("[读者%d] 离开，剩余读者数 = %d\n", id, readcount);
    sem_post(&rmutex);

    free(arg);
    return NULL;
}

/* ---------------- 写者线程 ---------------- */
void *writer(void *arg)
{
    int id = *(int *)arg;

    /* 1. 登记：第一个写者锁住入口 w，后续到达的读者只能排队 */
    sem_wait(&wmutex);
    writecount++;
    if (writecount == 1)
        sem_wait(&w);
    printf("[写者%d] 请求写入，等待写者数 = %d\n", id, writecount);
    sem_post(&wmutex);

    /* 2. 独占资源（r 保证写-写互斥、写-读互斥） */
    sem_wait(&r);
    printf("[写者%d] 开始写入 >>>>>\n", id);
    shared_data++;                 /* 修改共享数据 */
    usleep(300000);                /* 模拟写入耗时 0.3s */
    printf("[写者%d] 写入完成，数据 = %d <<<<<\n", id, shared_data);
    sem_post(&r);

    /* 3. 注销：最后一个写者放开入口 w */
    sem_wait(&wmutex);
    writecount--;
    if (writecount == 0)
        sem_post(&w);
    printf("[写者%d] 离开，剩余等待写者数 = %d\n", id, writecount);
    sem_post(&wmutex);

    free(arg);
    return NULL;
}

int main(void)
{
    pthread_t rtid[READER_NUM], wtid[WRITER_NUM];
    int i;

    /* 初始化信号量 */
    sem_init(&r,      0, 1);
    sem_init(&w,      0, 1);
    sem_init(&rmutex, 0, 1);
    sem_init(&wmutex, 0, 1);

    printf("====== 写者优先的读者-写者问题模拟开始 ======\n\n");

    /* 创建读者线程（错开时间，观察写者优先效果） */
    for (i = 0; i < READER_NUM; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&rtid[i], NULL, reader, id);
        usleep(50000);
    }

    /* 创建写者线程 */
    for (i = 0; i < WRITER_NUM; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&wtid[i], NULL, writer, id);
        usleep(100000);
    }

    /* 等待所有线程结束 */
    for (i = 0; i < READER_NUM; i++)
        pthread_join(rtid[i], NULL);
    for (i = 0; i < WRITER_NUM; i++)
        pthread_join(wtid[i], NULL);

    printf("\n====== 所有线程执行完毕，模拟结束 ======\n");

    /* 销毁信号量 */
    sem_destroy(&r);
    sem_destroy(&w);
    sem_destroy(&rmutex);
    sem_destroy(&wmutex);

    return 0;
}
