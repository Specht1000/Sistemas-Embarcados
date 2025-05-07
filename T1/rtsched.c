// Trabalho 1 da disciplina de Sistemas Embarcados
// Guilherme Martins Specht
// Última atualização: 06/05/2025

#include <ucx.h>

/* application tasks */
void task7(void)
{
	int32_t cnt = 800000;
	
	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task6(void)
{
	int32_t cnt = 700000;
	
	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task5(void)
{
	int32_t cnt = 600000;
	
	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task4(void)
{
	int32_t cnt = 500000;
	
	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task3(void)
{
	int32_t cnt = 400000;

	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task2(void)
{
	int32_t cnt = 300000;

	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task1(void)
{
	int32_t cnt = 200000;

	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

void task0(void)
{
	int32_t cnt = 100000;

	while (1) {
		printf("[task %d %ld]\n", ucx_task_id(), cnt++);
		ucx_task_wfi();
	}
}

struct our_priority_s {
	unsigned period;            // Período da Task
	unsigned capacity;          // Tempo de computação
	unsigned exec_time;         // Quanto tempo ainda precisa executar nesta instância
	unsigned next_activation;   // Deadline da próxima ativação (em ticks)
};

int32_t our_sched(void)
{
    struct node_s           *task_node;
    struct node_s           *chosen_node = NULL;
    struct tcb_s            *task;
    struct tcb_s            *chosen_task = NULL;
    struct our_priority_s   *priority;
    int32_t                  min_laxity = 0x7FFFFFFF;

    if (kcb->task_current) {
        task = kcb->task_current->data;
        if (task->state == TASK_RUNNING) {
            task->state = TASK_READY;
            if (task->rt_prio) {
                priority = (struct our_priority_s *)task->rt_prio;
                if (priority->exec_time > 0) {
                    priority->exec_time--;
                }
            }
        }
    }

    task_node = kcb->tasks->head->next;
    while (task_node != kcb->tasks->tail) {
        task = task_node->data;
        if (task->state == TASK_READY && task->rt_prio) {
            priority = (struct our_priority_s *)task->rt_prio;

            if (kcb->ticks >= priority->next_activation) {
                priority->next_activation = kcb->ticks + priority->period;
                priority->exec_time = priority->capacity;
            }

            if (priority->exec_time > 0) {
				float laxity = (priority->next_activation - (kcb->ticks + priority->exec_time));
                
				if (laxity < min_laxity) {
                    min_laxity    = laxity;
                    chosen_node   = task_node;
                    chosen_task   = task;
                }
            }
        }
        task_node = task_node->next;
    }

    if (!chosen_node)
        return -1;

    kcb->task_current    = chosen_node;
    chosen_task->state   = TASK_RUNNING;
    return chosen_task->id;
}

/* 
 * TESTE 1 - BAIXA CARGA DE CPU
 * Tarefas com períodos bem espaçados, carga baixa.
 */
// task0: 1 / 5  = 0.20
// task1: 1 / 7  = 0.143
// task2: 1 / 8  = 0.125
// Total carga: = 0.468

int32_t app_main_test1(void)
{
	struct our_priority_s priorities[3] = {
		{.period = 5, .capacity = 1, .exec_time = 1, .next_activation = 0}, 
		{.period = 7, .capacity = 1, .exec_time = 1, .next_activation = 0}, 
		{.period = 8, .capacity = 1, .exec_time = 1, .next_activation = 0} 
	};

	ucx_task_spawn(task0, DEFAULT_STACK_SIZE); 
	ucx_task_spawn(task1, DEFAULT_STACK_SIZE); 
	ucx_task_spawn(task2, DEFAULT_STACK_SIZE); 
	ucx_task_spawn(task3, DEFAULT_STACK_SIZE); 
	ucx_task_spawn(task4, DEFAULT_STACK_SIZE); 

	kcb->rt_sched = our_sched;
	ucx_task_rt_priority(0, &priorities[0]); 
	ucx_task_rt_priority(1, &priorities[1]);
	ucx_task_rt_priority(2, &priorities[2]);
	
	return 1;
}


/*
 * TESTE 2 - CARGA MODERADA
 * Tarefas com períodos médios, ainda factível.
 */
// task0: 2 / 5  = 0.40
// task1: 2 / 10 = 0.20
// task2: 3 / 15 = 0.20
// Total carga: = 0.80

int32_t app_main_test2(void)
{
	struct our_priority_s priorities[3] = {
		{.period = 5, .capacity = 2, .exec_time = 2, .next_activation = 0},
		{.period = 10, .capacity = 2, .exec_time = 2, .next_activation = 0},
		{.period = 15, .capacity = 3, .exec_time = 3, .next_activation = 0}
	};
	
	ucx_task_spawn(task0, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task1, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task2, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task3, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task4, DEFAULT_STACK_SIZE);

	kcb->rt_sched = our_sched;
	ucx_task_rt_priority(0, &priorities[0]);
	ucx_task_rt_priority(1, &priorities[1]);
	ucx_task_rt_priority(2, &priorities[2]);
	
	return 1;
}

/*
 * TESTE 3 – CARGA ALTA
 * Tarefas com períodos médios, ainda factível.
 */
// task0: 4 / 10  = 0.40
// task1: 9 / 20 = 0.45
// task2: 3 / 50 = 0.06
// task3: 8 / 200 = 0.04
// Total carga: = 0.95
int32_t app_main_test3(void)
{
    struct our_priority_s prio[4] = {
        {.period = 10,  .capacity = 4, .exec_time = 4, .next_activation = 0},
        {.period = 20,  .capacity = 9, .exec_time = 9, .next_activation = 0},
        {.period = 50,  .capacity = 3, .exec_time = 3, .next_activation = 0},
        {.period = 200, .capacity = 8, .exec_time = 8, .next_activation = 0}
    };

    ucx_task_spawn(task0, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task1, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task2, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task3, DEFAULT_STACK_SIZE);
	ucx_task_spawn(task4, DEFAULT_STACK_SIZE);

    kcb->rt_sched = our_sched;
    ucx_task_rt_priority(0, &prio[0]);
    ucx_task_rt_priority(1, &prio[1]);
    ucx_task_rt_priority(2, &prio[2]);
    ucx_task_rt_priority(3, &prio[3]);

    return 1;
}

/*
 * TESTE 4 – Muitas Tasks RT
 * 7 Tasks RT e 1 BE
 * Cada tarefa RT: C = 1, P = 14  Uᵢ = 1/14 = 0.0714
 * ΣU = 7·(1/14) = 0.5
 */
int32_t app_main_test4(void)
{
    /* 7 RT-priorities todas idênticas */
    struct our_priority_s prio[7] = {
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 },
        { .period = 14, .capacity = 1, .exec_time = 1, .next_activation = 0 }
    };

    /* 7 tarefas RT */
    ucx_task_spawn(task0, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task1, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task2, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task3, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task4, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task5, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task6, DEFAULT_STACK_SIZE); 
    ucx_task_spawn(task7, DEFAULT_STACK_SIZE);

    kcb->rt_sched = our_sched;

    /* associa prioridade RT às 7 primeiras tarefas */
    ucx_task_rt_priority(0, &prio[0]);
    ucx_task_rt_priority(1, &prio[1]);
    ucx_task_rt_priority(2, &prio[2]);
    ucx_task_rt_priority(3, &prio[3]);
    ucx_task_rt_priority(4, &prio[4]);
    ucx_task_rt_priority(5, &prio[5]);
    ucx_task_rt_priority(6, &prio[6]);

    return 1;
}

/*
 * TESTE 5 – INFACTÍVEL (U > 1)
 * task0: C0 = 2, P0 = 3  U0 = 0.667
 * task1: C1 = 2, P1 = 4  U1 = 0.500
 * task2: C2 = 2, P2 = 5  U2 = 0.400
 * Soma U = 1.567 > 1
 * + 2 tarefas BE para preencher o spawn
 */
int32_t app_main_test5(void)
{
    struct our_priority_s prio[3] = {
        { .period = 3, .capacity = 2, .exec_time = 2, .next_activation = 0 },
        { .period = 4, .capacity = 2, .exec_time = 2, .next_activation = 0 },
        { .period = 5, .capacity = 2, .exec_time = 2, .next_activation = 0 }
    };

    ucx_task_spawn(task0, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task1, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task2, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task3, DEFAULT_STACK_SIZE);
    ucx_task_spawn(task4, DEFAULT_STACK_SIZE);

    kcb->rt_sched = our_sched;
    ucx_task_rt_priority(0, &prio[0]);
    ucx_task_rt_priority(1, &prio[1]);
    ucx_task_rt_priority(2, &prio[2]);

    return 1;
}


int32_t app_main(void)
{
	app_main_test1();
	// app_main_test2();
	// app_main_test3();
	// app_main_test4();
	// app_main_test5();

	return 1;
}
