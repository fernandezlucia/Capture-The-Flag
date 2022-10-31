#include "equipo.h"

/**
 * @brief obtengo la direccion correspondiente a apuntar desde pos1 a pos2
 * por ejemplo: (5,5) -> (1,1) => ARRIBA o IZQUIERDA
 * @param pos1 Coordenada desde donde apuntamos
 * @param pos2 Coordenada hacia donde apuntamos
 * @return direccion a la que tenemos que avanzar.
 */
direccion Equipo::apuntar_a(coordenadas pos1, coordenadas pos2) {
	if(pos1.first > pos2.first) return IZQUIERDA;
	if (pos1.first < pos2.first) return DERECHA;
	// Estoy en la misma COL.
	if(pos1.second > pos2.second) return ARRIBA;
	if(pos1.second < pos2.second) return ABAJO;

	return ABAJO;
}


void Equipo::jugador(int nro_jugador) {
	//
	// ...
	//
	while(!this->belcebu->termino_juego()) { // Chequear que no haya una race condition en gameMaster
		switch(this->strat) {
			//SECUENCIAL,RR,SHORTEST,USTEDES
			case(SECUENCIAL): {
				int finalizador = -1;
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo);
					if(belcebu->ronda_actual() > 0){ 
						sem_wait(&belcebu->ronda_anterior_finalizada);
						sem_post(&belcebu->ronda_anterior_finalizada);
					}
                }

                coordenadas coords_bandera = buscar_bandera_contraria();
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);

                //parte critica: moverse y esperar barrera
                moverse.lock();
				if(!this->belcebu->termino_juego()){
					belcebu->mover_jugador(proxima_dir, nro_jugador);
				}
                cant_jugadores_que_ya_jugaron++;
                if(cant_jugadores_que_ya_jugaron == cant_jugadores){
					finalizador = nro_jugador;
					if(belcebu->ronda_actual() > 0 && this->equipo == ROJO) sem_post(&belcebu->ronda_anterior_finalizada);
                }
                moverse.unlock();
                //fin parte critica
				
				if(cant_jugadores_que_ya_jugaron == cant_jugadores && finalizador == nro_jugador) {
					cant_jugadores_que_ya_jugaron = 0;
					this->belcebu->termino_ronda(this->equipo);
					for (int i = 0; i < cant_jugadores; i++) sem_post(&barrier);
				}

				sem_wait(&barrier);
				if(this->equipo == AZUL) {
					if(todos_terminaron.fetch_add(1) >= cant_jugadores - 1){
						todos_terminaron.fetch_sub(cant_jugadores);
						sem_post(&belcebu->ronda_anterior_finalizada);
					}
					sem_post(&belcebu->turno_rojo);
                } else {
                    sem_post(&belcebu->turno_azul);
                }

				break;
			}	
			case(RR): {
				int finalizador = -1;
				// belcebu->mutexes_rr_azules
				if(equipo == ROJO) sem_wait(&belcebu->mutexes_rr_rojos[nro_jugador]);
				else sem_wait(&belcebu->mutexes_rr_azules[nro_jugador]);

 				if(quantum_restante > 0) {
					coordenadas coords_bandera = buscar_bandera_contraria(); // ejecutar en la creacion del equipo
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					belcebu->mover_jugador(proxima_dir, nro_jugador); // sin mutex, sabemos que esta bloqueado
					quantum_restante--;
					if(this->equipo == AZUL) {
						sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
					}else {
						sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);
					}
				} else {
					finalizador = nro_jugador;
					quantum_restante = quantum;
					this->belcebu->termino_ronda(this->equipo);
				}
				if(nro_jugador == finalizador) {
					if(this->equipo == AZUL){
						sem_post(&belcebu->mutexes_rr_rojos[0]);	
					} else {
						sem_post(&belcebu->mutexes_rr_azules[0]);
					}
				}
				break;
			}
			case(SHORTEST):
				
				//molinetes
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                    sem_post(&belcebu->turno_azul);	
                } else {
                    sem_wait(&belcebu->turno_rojo); 
                    sem_post(&belcebu->turno_rojo);
                }

				if(belcebu->soy_el_mas_cercano(nro_jugador,this->equipo)){
					cout << "Soy el mas cercano, mi numero es: " << nro_jugador << endl;
					coordenadas coords_bandera = buscar_bandera_contraria(); // Hay que paralelizar esto, cada uno busca en un sector
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					
					belcebu->mover_jugador(proxima_dir, nro_jugador);
					this->belcebu->termino_ronda(this->equipo);
				}

				break;

			case(USTEDES):
				// ideas
				// Movimientos random
				// Longest Job First
				// Un jugador tiene mas quantum que los demas

				break;
			default:
				break;
		}	
		// Termino ronda ? Recordar llamar a belcebu...
		// OJO. Esto lo termina un jugador... 
		//
		// ...
		//
	}
	switch(this->strat){
		case(SECUENCIAL): {
			sem_post(&belcebu->ronda_anterior_finalizada);
			sem_post(&belcebu->turno_rojo);
			sem_post(&barrier);
			break;
		}
		case(RR): {
			if(this->equipo == AZUL){
				sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);	
			} else {
				sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
			}
			break;
		}
		default: {

		}
	}
	
	
}

Equipo::Equipo(gameMaster *belcebu, color equipo, 
		estrategia strat, int cant_jugadores, int quantum, vector<coordenadas> posiciones) {

	this->belcebu = belcebu;
	this->equipo = equipo;
	this->contrario = (equipo == ROJO)? AZUL : ROJO;
	this->bandera_contraria = (equipo==ROJO)? BANDERA_AZUL : BANDERA_ROJA;
	this->strat = strat;
	this->quantum = quantum;
	this->quantum_restante = quantum;
	this->cant_jugadores = cant_jugadores;
	this->posiciones = posiciones;

	for(int i = 0; i < cant_jugadores; i++){
		sem_t semaphore;
		if(equipo == ROJO) belcebu->mutexes_rr_rojos.emplace_back(semaphore);
		else belcebu->mutexes_rr_azules.emplace_back(semaphore);
		if(i == 0){
			if(equipo == ROJO) sem_init(&(belcebu->mutexes_rr_rojos[i]), 0, 1);
			else sem_init(&(belcebu->mutexes_rr_azules[i]), 0, 0);
		} else {
			if(equipo == ROJO) sem_init(&(belcebu->mutexes_rr_rojos[i]), 0, 0);
			else sem_init(&(belcebu->mutexes_rr_azules[i]), 0, 0);
		}
	}
	
	for(int i = 0; i < cant_jugadores; i++){
		jugadores_ya_jugaron.push_back(0);
	}

	sem_init(&barrier, 0, 0);
	
    //
	// ...
	//

	//Si necesitamos mas semaforos van aca

	// interpretamos en la consigna que de no alcanzar para cubrir a todos los jugadores con el quantum,
	// todos se mueven una vez.
	if(quantum < cant_jugadores){
		quantum = cant_jugadores;
		quantum_restante = quantum;
	}

    //Iniciar su respectivo semaforo.
	if(equipo == AZUL){
		sem_init(&belcebu->turno_azul, 0, 0);
	} else {
		sem_init(&belcebu->turno_rojo, 0, cant_jugadores);
	}
}

void Equipo::comenzar() {
	// Arranco cuando me toque el turno 
	// TODO: Quien empieza ? 
	//
	// ...
	//
	
	// Creamos los jugadores
	for(int i=0; i < cant_jugadores; i++) {
		jugadores.emplace_back(thread(&Equipo::jugador, this, i)); 
	}

}

void Equipo::terminar() {
	for(auto &t : jugadores){
		t.join();
	}
}

coordenadas Equipo::buscar_bandera_contraria() {
	return contrario == ROJO ? belcebu->pos_bandera_roja : belcebu->pos_bandera_azul;
}
