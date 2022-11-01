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




	//Busqueda de bandera
	coordenadas pos_actual;
	pos_actual.first = nro_jugador+1;  //me interesa que el jugador 0 mire la 1ra fila.
	pos_actual.second = 1;

	while(!bandera_found){

		//entra si hay filas que mirar en el tablero
		while(pos_actual.first <= this->belcebu->getTamx() && !bandera_found){

			if(this->belcebu->es_posicion_bandera(pos_actual, this->contrario)){
				bandera_found = true;
				pos_bandera_contraria = pos_actual;
				//cout << "Bandera " 
				//     << ((this->equipo == AZUL) ? "Roja" : "Azul") 
				//	 << " encontrada por jugador " 
				//	 << nro_jugador << " en: ("
				//	 << pos_bandera_contraria.first << ", " << pos_bandera_contraria.second << "). " << endl;
			}

			pos_actual.second = pos_actual.second + 1;
			if(pos_actual.second > this->belcebu->getTamy()){
				pos_actual.second = 1;
				pos_actual.first += this->cant_jugadores;
			}

			

		}

	}


	//Estrategias
	while(!this->belcebu->termino_juego()) { // Chequear que no haya una race condition en gameMaster

		switch(this->strat) {
			//SECUENCIAL,RR,SHORTEST,USTEDES
			case(SECUENCIAL): {

                //molinetes
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                    sem_post(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo);
                    sem_post(&belcebu->turno_rojo);
                }

                coordenadas coords_bandera = buscar_bandera_contraria(); // Hay que paralelizar esto, cada uno busca en un sector
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);

                //parte critica: moverse y esperar barrera
                moverse.lock();
                belcebu->mover_jugador(proxima_dir, nro_jugador);
                cant_jugadores_que_ya_jugaron++;
                if(cant_jugadores_que_ya_jugaron == cant_jugadores){
                    sem_wait(&barrier2); //trabar barrera2
                    sem_post(&barrier); //liberar barrera
                }
                moverse.unlock();
                //fin parte critica

                sem_wait(&barrier);
                sem_post(&barrier);

                //ultimo decrementa value, y termina la ronda

                fafa.lock();
                cant_jugadores_que_ya_jugaron--;
                if (cant_jugadores_que_ya_jugaron == 0){
                    this->belcebu->termino_ronda(this->equipo);
                    sem_wait(&barrier);
                    sem_post(&barrier2);
                }
                fafa.unlock();

                sem_wait(&barrier2);
                sem_post(&barrier2);
				//}

				break;
			}	
			case(RR):
				
				// mutex que solo entra uno
				
				sem_wait(&mutexes_rr[nro_jugador]);

				//molinetes
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                    sem_post(&belcebu->turno_azul);	
                } else {
                    sem_wait(&belcebu->turno_rojo); 
                    sem_post(&belcebu->turno_rojo);
                }

				if(quantum_restante > 0){
					coordenadas coords_bandera = buscar_bandera_contraria(); // Hay que paralelizar esto, cada uno busca en un sector
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					
					belcebu->mover_jugador(proxima_dir, nro_jugador);
					
					quantum_restante--;
					sem_post(&mutexes_rr[(nro_jugador + 1) % cant_jugadores]);
				} else {
					quantum_restante = quantum;
					sem_post(&mutexes_rr[0]);
					this->belcebu->termino_ronda(this->equipo);
					sem_post(&barrier);
				}
				
				break;

			case(SHORTEST):

				//molinetes
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                    sem_post(&belcebu->turno_azul);	
                } else {
                    sem_wait(&belcebu->turno_rojo); 
                    sem_post(&belcebu->turno_rojo);
                }

				if(!belcebu->soy_el_mas_cercano(nro_jugador,this->equipo)){
					sem_wait(&barrier);
				} else {
					cout << "Soy el mas cercano, mi numero es: " << nro_jugador << endl;
					coordenadas coords_bandera = buscar_bandera_contraria();
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					
					belcebu->mover_jugador(proxima_dir, nro_jugador);
					this->belcebu->termino_ronda(this->equipo);
				}
				sem_post(&barrier);

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
		mutexes_rr.emplace_back(semaphore);
		if(i == 0){
			sem_init(&mutexes_rr[i], 0, 1);
		} else {
			sem_init(&mutexes_rr[i], 0, 0);
		}
	}
	
	for(int i = 0; i < cant_jugadores; i++){
		jugadores_ya_jugaron.push_back(0);
	}

	sem_init(&barrier, 0, 0);
    sem_init(&barrier2, 0, 1);
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
		sem_init(&belcebu->turno_rojo, 0, 1);
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
	//return contrario == ROJO ? belcebu->pos_bandera_roja : belcebu->pos_bandera_azul;
	return pos_bandera_contraria;
}
