#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<math.h>

double err_max; // valor maximo do erro da integral
double integral = 0.0; // valor da integral

pthread_mutex_t mutex;

// Estrutura para guardar o início e fim de um intervalo de função
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

// A pilha está cheia quando o topo é a última posição do tamanho
int isFull(Pilha *pilha) {
    return pilha->top == pilha->length - 1;
}

// A pilha está cheia quando o topo é -1
int isEmpty(Pilha *pilha) {
    return pilha->top == -1;
}

// Adiciona um novo intervalo na pilha. Se estiver cheia, expande o seu tamanho
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

// Pega o intervalo do topo da pilha
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

    interv = peek(intervalos);
    pop(intervalos);

    p_medio_a = (interv.b + interv.a) / 2;
    p_medio_b = (p_medio_a + interv.a) / 2;
    p_medio_c = (interv.b + p_medio_a) / 2;

    ret_a = mathFunction(p_medio_a) * (interv.b - interv.a);
    ret_b = mathFunction(p_medio_b) * (p_medio_a - interv.a);
    ret_c = mathFunction(p_medio_c) * (interv.b - p_medio_a);

    diff = ret_a - (ret_b + ret_c);
    if(diff < 0) diff = diff * -1.0;

    if(diff < err_max) {
        integral += ret_b + ret_c;
    } else {
        novos[0].a = interv.a;
        novos[0].b = p_medio_a;
        novos[1].a = p_medio_a;
        novos[1].b = interv.b;

        push(intervalos, novos[0]);
        push(intervalos, novos[1]);
    }
}

void *integra(void *intervalos) {

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
    int nthreads; // número de threads que irão trabalhar
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

    for(i = 0; i < nthreads; i++) {
        if(pthread_create(&tid_sistema[i], NULL, integra, (void *) intervalos)) {
            printf("Erro de criação de threads\n");
            exit(EXIT_FAILURE);
        }
    }

    while(!isEmpty(intervalos)) {
        calculaIntegral(intervalos);
    }

    printf("O valor da integral é aproximadamente: %.5lf\n", integral);

    return 0;
}