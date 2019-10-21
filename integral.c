#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>

double err_max; // valor maximo do erro da integral

// Estrutura para guardar o início e fim de um intervalo de função
typedef struct INTERVALO {
    double a;   // inicio do intervalo
    double b;   // fim do intervalo
} Intervalo;

// Função matemática da qual quer se obter a integral
double mathFunction(int x) {
    return x + 1;
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

void calculaIntegral(Pilha *intervalos) {
    Intervalo interv; // Guarda o intervalo que está se fazendo o cálculo da integral
    double p_medio; // Ponto médio do intervalo analisado
    double ret_a, ret_b, ret_c; // Área do retângulo maior e dos retângulos menores, respectivamente

    interv = peek(intervalos);
    pop(intervalos);

    p_medio = (interv.b - interv.a) / 2;

    ret_a = mathFunction(p_medio) * (interv.b - interv.a);
    ret_b = mathFunction
}

int main(int argc, char *argv[]) {
    Intervalo inicial; // Guarda o intervalo inicial dado pelo usuário
    Pilha *intervalos = init(100); // Inicializando uma pilha de intervalos com 100 espaços

    if (argc < 4) {
        printf("Por favor, informe: %s <inicio do intervalo> <fim do intervalo> <erro maximo>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    inicial.a = atof(argv[1]);
    inicial.b = atof(argv[2]);
    err_max = atof(argv[2]);

    push(intervalos, inicial);

    while(!isEmpty(intervalos)) {
        calculaIntegral(intervalos);
    }

    return 0;
}