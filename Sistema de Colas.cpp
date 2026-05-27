/* Simulacion de tiempo discreto para un sistema Geo/Geo/m/FIFO/N. */

#include <stdio.h>
#include <stdlib.h>
#include <queue>

#include "lcgrand.cpp"

struct ResultadoSimulacion {
    long intervalos;
    long llegadas;
    long aceptados;
    long rechazados;
    long esperaron;
    double probabilidad_rechazo;
    double probabilidad_espera;
    double utilizacion;
    double numero_promedio_cola;
    double numero_promedio_sistema;
};

FILE *parametros;
FILE *resultados;

double prob_llegada;
double prob_servicio;
int    num_servidores;
int    capacidad_sistema;
long   num_intervalos;

int    servidores_ocupados;
std::queue<int> cola;
long   llegadas;
long   aceptados;
long   rechazados;
long   esperaron;
double area_servidores_ocupados;
double area_num_cola;
double area_num_sistema;

void inicializar(void);
void procesar_servicios(void);
void procesar_llegada(int intervalo);
void actualizar_estadisticas(void);
ResultadoSimulacion simular_geo_geo_m(void);
void reportar(ResultadoSimulacion resultado);
int  evento_bernoulli(double probabilidad, int flujo);

int main(void)
{
    parametros = fopen("param.txt", "r");
    resultados = fopen("result.txt", "w");

    if (parametros == NULL || resultados == NULL) {
        printf("No se pudieron abrir los archivos param.txt o result.txt.\n");
        return 1;
    }

    if (fscanf(parametros, "%lf %lf %d %d %ld", &prob_llegada, &prob_servicio,
               &num_servidores, &capacidad_sistema, &num_intervalos) != 5) {
        fprintf(resultados, "Error: param.txt debe contener p, q, m, N y numero de intervalos.\n");
        fclose(parametros);
        fclose(resultados);
        return 1;
    }

    if (prob_llegada < 0.0 || prob_llegada > 1.0 ||
        prob_servicio < 0.0 || prob_servicio > 1.0 ||
        num_servidores <= 0 || capacidad_sistema < num_servidores ||
        num_intervalos <= 0) {
        fprintf(resultados, "Error: parametros invalidos para el modelo Geo/Geo/m/FIFO/N.\n");
        fclose(parametros);
        fclose(resultados);
        return 1;
    }

    ResultadoSimulacion resultado = simular_geo_geo_m();
    reportar(resultado);

    fclose(parametros);
    fclose(resultados);

    return 0;
}

void inicializar(void)
{
    servidores_ocupados = 0;

    while (!cola.empty()) {
        cola.pop();
    }

    llegadas = 0;
    aceptados = 0;
    rechazados = 0;
    esperaron = 0;
    area_servidores_ocupados = 0.0;
    area_num_cola = 0.0;
    area_num_sistema = 0.0;
}

ResultadoSimulacion simular_geo_geo_m(void)
{
    inicializar();

    for (long intervalo = 1; intervalo <= num_intervalos; ++intervalo) {
        procesar_servicios();
        procesar_llegada((int) intervalo);
        actualizar_estadisticas();
    }

    ResultadoSimulacion resultado;
    resultado.intervalos = num_intervalos;
    resultado.llegadas = llegadas;
    resultado.aceptados = aceptados;
    resultado.rechazados = rechazados;
    resultado.esperaron = esperaron;

    if (llegadas > 0) {
        resultado.probabilidad_rechazo = (double) rechazados / (double) llegadas;
        resultado.probabilidad_espera = (double) esperaron / (double) llegadas;
    } else {
        resultado.probabilidad_rechazo = 0.0;
        resultado.probabilidad_espera = 0.0;
    }

    resultado.utilizacion = area_servidores_ocupados / ((double) num_intervalos * num_servidores);
    resultado.numero_promedio_cola = area_num_cola / (double) num_intervalos;
    resultado.numero_promedio_sistema = area_num_sistema / (double) num_intervalos;

    return resultado;
}

void procesar_servicios(void)
{
    int ocupados_inicio = servidores_ocupados;

    for (int servidor = 0; servidor < ocupados_inicio; ++servidor) {
        if (evento_bernoulli(prob_servicio, 1)) {
            --servidores_ocupados;
        }
    }

    while (!cola.empty() && servidores_ocupados < num_servidores) {
        cola.pop();
        ++servidores_ocupados;
    }
}

void procesar_llegada(int intervalo)
{
    int clientes_sistema;

    if (!evento_bernoulli(prob_llegada, 2)) {
        return;
    }

    ++llegadas;
    clientes_sistema = servidores_ocupados + (int) cola.size();

    if (clientes_sistema >= capacidad_sistema) {
        ++rechazados;
    } else if (servidores_ocupados < num_servidores) {
        ++aceptados;
        ++servidores_ocupados;
    } else {
        ++aceptados;
        ++esperaron;
        cola.push(intervalo);
    }
}

void actualizar_estadisticas(void)
{
    area_servidores_ocupados += servidores_ocupados;
    area_num_cola += (double) cola.size();
    area_num_sistema += servidores_ocupados + (double) cola.size();
}

void reportar(ResultadoSimulacion resultado)
{
    double carga_aproximada = prob_llegada / (num_servidores * prob_servicio);

    fprintf(resultados, "Modelo de Tiempo Discreto Geo/Geo/m/FIFO/N\n\n");
    fprintf(resultados, "Probabilidad de llegada p%17.6f\n", prob_llegada);
    fprintf(resultados, "Probabilidad de servicio q%16.6f\n", prob_servicio);
    fprintf(resultados, "Numero de servidores m%18d\n", num_servidores);
    fprintf(resultados, "Capacidad finita N%21d clientes\n", capacidad_sistema);
    fprintf(resultados, "Numero de intervalos simulados%9ld\n", resultado.intervalos);
    fprintf(resultados, "Carga aproximada p/(m*q)%14.6f\n\n", carga_aproximada);

    fprintf(resultados, "Llegadas generadas%25ld\n", resultado.llegadas);
    fprintf(resultados, "Clientes aceptados%24ld\n", resultado.aceptados);
    fprintf(resultados, "Clientes rechazados%23ld\n", resultado.rechazados);
    fprintf(resultados, "Clientes que esperaron%21ld\n\n", resultado.esperaron);

    fprintf(resultados, "Probabilidad de rechazo%17.6f\n", resultado.probabilidad_rechazo);
    fprintf(resultados, "Probabilidad de espera%18.6f\n", resultado.probabilidad_espera);
    fprintf(resultados, "Utilizacion promedio%20.6f\n", resultado.utilizacion);
    fprintf(resultados, "Numero promedio en cola%17.6f\n", resultado.numero_promedio_cola);
    fprintf(resultados, "Numero promedio en sistema%14.6f\n", resultado.numero_promedio_sistema);
}

int evento_bernoulli(double probabilidad, int flujo)
{
    return lcgrand(flujo) < probabilidad;
}
