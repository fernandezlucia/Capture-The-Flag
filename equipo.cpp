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
				(this->equipo == AZUL) ? (sem_wait(&belcebu->turno_azul)) : (sem_wait(&belcebu->turno_rojo));
				
				// el que empiece libre, arranca. cuando vuelva a entrar, el lock lo va a frenar.
				if(cant_jugadores_que_ya_jugaron < cant_jugadores) {
					coordenadas coords_bandera = buscar_bandera_contraria(); // Hay que paralelizar esto, cada uno busca en un sector
					direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					
					printlock.lock();
					belcebu->mover_jugador(proxima_dir, nro_jugador);
					printlock.unlock();

					int p = value.fetch_add(1);
					if(cant_jugadores - 1 > p) {
						sem_wait(&barrier);
					}
					sem_post(&barrier);
					fafa.lock();
					if(p == cant_jugadores - 1) {
						value.fetch_sub(p + 1);
						this->belcebu->termino_ronda(this->equipo);
					}
					fafa.unlock();
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
	sem_init(&barrier, 0, 0);
	//
	// ...
	//

	//Si necesitamos mas semaforos van aca
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
