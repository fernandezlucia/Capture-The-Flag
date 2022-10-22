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
	cout << "Ya estamos en la bandera" << endl;
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

				//este semaforo es para determinar que equipo esta jugando
				sem_t turno = (this->equipo == AZUL) ? (this->belcebu->turno_azul) : (this->belcebu->turno_rojo);
				sem_wait(&turno);
				
				// el que empiece libre, arranca. cuando vuelva a entrar, el lock lo va a frenar.
				
				if(cant_jugadores_que_ya_jugaron < cant_jugadores) {
					coordenadas coords_bandera = buscar_bandera_contraria(); // Hay que paralelizar esto, cada uno busca en un sector
					direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					
					belcebu->mover_jugador(proxima_dir, nro_jugador);
					//posiciones[nro_jugador] = belcebu->proxima_posicion(posiciones[nro_jugador], proxima_dir);
					
					printlock.lock();
					cant_jugadores_que_ya_jugaron++;
					
					
					if(cant_jugadores_que_ya_jugaron == cant_jugadores) {
						cant_jugadores_que_ya_jugaron = 0;
						this->belcebu->termino_ronda(this->equipo);
					}
					
					printlock.unlock();

					if(cant_jugadores > cant_jugadores_que_ya_jugaron) sem_wait(&barrier);
					sem_post(&barrier);
				}

				break;
			}	
			case(RR):
				//
				// ...
				//
				break;

			case(SHORTEST):
				//
				// ...
				//
				break;

			case(USTEDES):
				//
				// ...
				//
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
	//
	// ...
	//

	//Si necesitamos mas semaforos van aca
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
	return contrario == ROJO ? belcebu->pos_bandera_roja : belcebu->pos_bandera_azul;
}
