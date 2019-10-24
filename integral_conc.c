#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>

double err_max; // valor maximo do erro da integral
double integral = 0.0; // valor da integral

pthread_mutex_t mutex;
pthread_cond_t cond_e, cond_f; // condição para as threads quando a pilha está vazia e cheia, respectivamente
int nthreads; // número de threads que irão trabalhar
int thread_esperando = 0; // número de threads que estão bloqueadas com cond_e
int integral_finalizada = 0; // boolean para indicar se a integral foi finalizada, que é depois
                            // de todas as threads terem sido bloqueadas menos a ultima e esta
                            // saiu do loop de calcular a integral

// Estrutura para guardar o início e fim de um intervalo de uma função
typedef struct INTERVALO {
    double a;   // inicio do intervalo
    double b;   // fim do intervalo
} Intervalo;

// Função matemática da qual quer se obter a integral
double mathFunction(double x) {
    return pow(x, 2.0);
}

// Estrutura de dados da pilha
typedef struct PILHA {
    int top;
    int length;
    Intervalo *array;
} Pilha;

// Aloca memória para uma pilha dada um tamanho. Ela é inicializada vazia
Pilha *init(int length) {
    Pilha *pilha = (Pilha *)malloc(sizeof(Pilha));
    if(pilha == NULL) { printf("Erro de malloc na criação da pilha\n"); exit(EXIT_FAILURE); }

    pilha->length = length;
    pilha->top = -1;
    pilha->array = (Intervalo *)malloc(pilha->length * sizeof(Intervalo));
    if(pilha->array == NULL) { printf("Erro de malloc na criação do conteúdo da pilha\n"); exit(EXIT_FAILURE); }

    return pilha;
}

// A pilha está cheia quando o topo tiver o valor de tamanho - 1
int isFull(Pilha *pilha) {
    return pilha->top == pilha->length - 1;
}

// A pilha está cheia quando o topo é -1
int isEmpty(Pilha *pilha) {
    return pilha->top == -1;
}

// Adiciona um novo intervalo na pilha. Se estiver cheia, termina o programa
void push(Pilha *pilha, Intervalo elem) {
    if (isFull(pilha)) {
        printf("A pilha esta cheia! O tamanho da pilha nao eh suficiente para resolver essa integral.\n");
        exit(EXIT_FAILURE);
    }
    
    pilha->top++;
    pilha->array[pilha->top] = elem;
}

// Remove um intervalo da pilha decrementando o topo
void pop(Pilha *pilha) {
    if (isEmpty(pilha)) {
        return;
    }

    pilha->top--;
}

// Retorna o intervalo do topo da pilha
Intervalo peek(Pilha *pilha) {
    if (isEmpty(pilha)) {
        Intervalo vazio;
        vazio.a = 0.0;
        vazio.b = 0.0;
        return vazio;
    }

    return pilha->array[pilha->top];
}

// Calcula a área de um intervalo debaixo de uma dada função pelo método retangular
// utilizando a estratégia de quadratura adaptativa. Se a diferença obtida entre o retângulo
// maior e os retângulos menores for menor que o erro estipulado então a área dos retângulos 
// menores são adicionados no somatório da integral, caso contrário os intervalos de tais são 
// adicionados na pilha
void calculaIntegral(Pilha *intervalos) {
    Intervalo interv; // Guarda o intervalo que está se fazendo o cálculo da integral, que é quem estiver no topo da pilha
    Intervalo novos[2]; // Guarda os novos intervalos que serão introduzidos na pilha
    double p_medio_a, p_medio_b, p_medio_c; // Ponto médio de seus respectivos retângulos
    double ret_a, ret_b, ret_c; // Área do retângulo maior e dos retângulos menores, respectivamente
    double diff; // Diferença da área entre o retângulo maior e os retângulos menores

    pthread_mutex_lock(&mutex);
    // pode ser que a thread tenha entrado na função para calcular a integral mas outras threads
    // pegaram todos os intervalos restantes e não sobrou nenhum para essa thread calcular
    if (isEmpty(intervalos)) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    interv = peek(intervalos);
    pop(intervalos);
    pthread_cond_signal(&cond_f);
    pthread_mutex_unlock(&mutex);

    p_medio_a = (interv.b + interv.a) / 2;
    p_medio_b = (p_medio_a + interv.a) / 2;
    p_medio_c = (interv.b + p_medio_a) / 2;

    ret_a = mathFunction(p_medio_a) * (interv.b - interv.a);
    ret_b = mathFunction(p_medio_b) * (p_medio_a - interv.a);
    ret_c = mathFunction(p_medio_c) * (interv.b - p_medio_a);

    diff = ret_a - (ret_b + ret_c);
    if(diff < 0) diff = diff * -1.0;

    if(diff < err_max) {
        pthread_mutex_lock(&mutex);
        integral += ret_b + ret_c;
        pthread_mutex_unlock(&mutex);
    } else {
        novos[0].a = interv.a;
        novos[0].b = p_medio_a;
        novos[1].a = p_medio_a;
        novos[1].b = interv.b;

        pthread_mutex_lock(&mutex);

        // pode ser que a pilha tenha ficado cheia durante os calculos da thread, então
        // ela deve ser bloqueada até alguma outra thread retirar um intervalo da pilha
        while(isFull(intervalos)) {
            pthread_cond_wait(&cond_f, &mutex);
        }
        push(intervalos, novos[0]);
        push(intervalos, novos[1]);
        pthread_cond_broadcast(&cond_e);
        pthread_mutex_unlock(&mutex);
    }
}

void *integra(void *intervalos) {
    intervalos = (Pilha *)intervalos;

    while(!isEmpty(intervalos)) {
        calculaIntegral(intervalos);

        // se a thread entrar nesse if quer dizer que não tem mais nenhum intervalo na
        // pilha para se analisar, porém pode ser que outra thread esteja analisando um
        // e pode ser que ela bote novos intervalos na pilha, indicando que a integral
        // não tenha acabado ainda. Entretanto, para indicar que tenha acabado bloqueia-se
        // todas as threads menos a última, sendo que se realmente acabou ela sai do loop e
        // desbloqueia as outras threads.
        pthread_mutex_lock(&mutex);
        while(!integral_finalizada && isEmpty(intervalos) && thread_esperando < nthreads - 1) {
            thread_esperando++;
            pthread_cond_wait(&cond_e, &mutex);
            thread_esperando--;
        }
        pthread_mutex_unlock(&mutex);
    }
    integral_finalizada = 1;
    pthread_cond_broadcast(&cond_e);

    pthread_exit(NULL);
}

// Para o cálculo de uma integral a partir de um intervalo e erro dado e por meio do método
// indicado no trabalho (integração numérica retangular com quadratura adaptativa) utilizamos
// uma estrutura de dados, no caso uma pilha, para guardar os intervalos, sendo que primeiramente
// ela guarda somente o intervalo inicial e conforme o programa vai fazendo os cálculos ele
// vai tirando e colocando novos intervalos nesta pilha enquanto ele não atingir um valor menor 
// que o erro dado e para somente quando a pilha não tiver mais nenhum intervalo para calcular
int main(int argc, char *argv[]) {
    Intervalo inicial; // Guarda o intervalo inicial dado pelo usuário
    Pilha *intervalos = init(100); // Inicializando uma pilha de intervalos com 100 espaços
    pthread_t *tid_sistema; // threads que irão realizar o cálculo da integral
    int i;

    if(argc < 5) {
        printf("Por favor, informe: %s <inicio do intervalo> <fim do intervalo> <erro maximo> <numero de threads>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    inicial.a = atof(argv[1]);
    inicial.b = atof(argv[2]);
    err_max = atof(argv[3]);
    nthreads = atoi(argv[4]);

    if(inicial.b <= inicial.a) {
        printf("O fim do intervalo eh menor que o inicio. Por favor, informe um fim que seja maior que o inicio.\n");
        exit(EXIT_FAILURE);    
    }

    push(intervalos, inicial);

    tid_sistema = (pthread_t *)malloc(nthreads * sizeof(pthread_t));
    if(tid_sistema == NULL) { printf("Erro de malloc nas threads\n"); exit(EXIT_FAILURE); }

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_e, NULL);
    pthread_cond_init(&cond_f, NULL);

    for(i = 0; i < nthreads; i++) {
        if(pthread_create(&tid_sistema[i], NULL, integra, (void *) intervalos)) {
            printf("Erro de criação de threads\n");
            exit(EXIT_FAILURE);
        }
    }

    for(i = 0; i < nthreads; i++) {
        if(pthread_join(tid_sistema[i], NULL)) {
            printf("Erro de espera das threads\n");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_e);
    pthread_cond_destroy(&cond_f);

    printf("O valor da integral é aproximadamente: %.5lf\n", integral);

    free(tid_sistema);
    free(intervalos->array);
    free(intervalos);

    return 0;
}