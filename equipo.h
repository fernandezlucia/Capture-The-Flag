#ifndef EQUIPO_H
#define EQUIPO_H

#include <semaphore.h>
#include <vector>
#include <thread>
#include "definiciones.h"
#include "gameMaster.h"
#include <time.h>
#include <atomic>

using namespace std;

class Equipo {
	private:

		// Atributos Privados 
		gameMaster *belcebu; 
		color contrario, equipo, bandera_contraria;
		estrategia strat;
		int cant_jugadores, quantum, quantum_restante;
		vector<thread> jugadores;
		int cant_jugadores_que_ya_jugaron = 0;
		vector<coordenadas> posiciones;
		coordenadas pos_bandera_contraria;

		// Nuestros
		mutex moverse;
		mutex terminacion_de_ronda;
		sem_t barrier;
		atomic_int todos_terminaron{0};
		sem_t lejanos;

		struct timespec tiempo_antes, tiempo_despues;
		double tiempo_en_encontrar_bandera;

		bool bandera_found = false;

		vector<int> jugadores_ya_jugaron;
		vector<int> quantums_por_jugador;
		
		// Métodos privados 
		direccion apuntar_a(coordenadas pos2, coordenadas pos1);
		void jugador(int nro_jugador);
		coordenadas get_pos_bandera_contraria();
		void buscar_bandera_naif();

		// Nuestros
		void reiniciar_quantums();
		
	public:
		Equipo(gameMaster *belcebu, color equipo, 
				estrategia strat, int cant_jugadores, int quantum, vector<coordenadas> posiciones);
		void comenzar();
		void terminar();
		// crear jugadores

};
#endif // EQUIPO_H
