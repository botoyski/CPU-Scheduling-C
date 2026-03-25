#include <stdio.h>

#include "queue.h"

static int test_init_invalid_capacity(void)
{
    IntQueue q;
    if (queue_init(&q, 0))
    {
        queue_free(&q);
        fprintf(stderr, "test_init_invalid_capacity failed\n");
        return 0;
    }
    return 1;
}

static int test_push_pop_fifo(void)
{
    IntQueue q;
    if (!queue_init(&q, 4))
    {
        fprintf(stderr, "test_push_pop_fifo setup failed\n");
        return 0;
    }

    int ok = 1;
    ok &= queue_push(&q, 10);
    ok &= queue_push(&q, 20);
    ok &= queue_push(&q, 30);

    int v = -1;
    ok &= queue_pop(&q, &v) && (v == 10);
    ok &= queue_pop(&q, &v) && (v == 20);
    ok &= queue_pop(&q, &v) && (v == 30);
    ok &= queue_empty(&q);

    queue_free(&q);

    if (!ok)
    {
        fprintf(stderr, "test_push_pop_fifo failed\n");
        return 0;
    }
    return 1;
}

static int test_full_and_overflow(void)
{
    IntQueue q;
    if (!queue_init(&q, 2))
    {
        fprintf(stderr, "test_full_and_overflow setup failed\n");
        return 0;
    }

    int ok = 1;
    ok &= queue_push(&q, 1);
    ok &= queue_push(&q, 2);
    ok &= queue_full(&q);
    ok &= !queue_push(&q, 3);

    queue_free(&q);

    if (!ok)
    {
        fprintf(stderr, "test_full_and_overflow failed\n");
        return 0;
    }
    return 1;
}

static int test_underflow(void)
{
    IntQueue q;
    if (!queue_init(&q, 2))
    {
        fprintf(stderr, "test_underflow setup failed\n");
        return 0;
    }

    int v = -1;
    int ok = !queue_pop(&q, &v);
    queue_free(&q);

    if (!ok)
    {
        fprintf(stderr, "test_underflow failed\n");
        return 0;
    }
    return 1;
}

static int test_wraparound(void)
{
    IntQueue q;
    if (!queue_init(&q, 3))
    {
        fprintf(stderr, "test_wraparound setup failed\n");
        return 0;
    }

    int ok = 1;
    int v;

    ok &= queue_push(&q, 1);
    ok &= queue_push(&q, 2);
    ok &= queue_pop(&q, &v) && (v == 1);
    ok &= queue_push(&q, 3);
    ok &= queue_push(&q, 4);

    ok &= queue_pop(&q, &v) && (v == 2);
    ok &= queue_pop(&q, &v) && (v == 3);
    ok &= queue_pop(&q, &v) && (v == 4);
    ok &= queue_empty(&q);

    queue_free(&q);

    if (!ok)
    {
        fprintf(stderr, "test_wraparound failed\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    int passed = 0;
    int total = 5;

    passed += test_init_invalid_capacity();
    passed += test_push_pop_fifo();
    passed += test_full_and_overflow();
    passed += test_underflow();
    passed += test_wraparound();

    printf("Queue tests passed: %d/%d\n", passed, total);
    return passed == total ? 0 : 1;
}
