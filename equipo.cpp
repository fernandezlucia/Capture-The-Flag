#include "equipo.h"
#include <random>
/**
 * @brief obtengo la direccion correspondiente a apuntar desde pos1 a pos2
 * por ejemplo: (5,5) -> (1,1) => ARRIBA o IZQUIERDA
 * @param pos1 Coordenada desde donde apuntamos
 * @param pos2 Coordenada hacia donde apuntamos
 * @return direccion a la que tenemos que avanzar.
 */
direccion Equipo::apuntar_a(coordenadas pos1, coordenadas pos2) { // 99,2 -> 99,99
	if (pos1.first > pos2.first) return IZQUIERDA;
	if (pos1.first < pos2.first) return DERECHA;
	// Estoy en la misma COL.
	if (pos1.second > pos2.second) return ARRIBA;
	if (pos1.second < pos2.second) return ABAJO;

	return ABAJO;
}


void Equipo::jugador(int nro_jugador) {
	
	//Busqueda de bandera
	coordenadas pos_actual;
	pos_actual.first = nro_jugador+1;  //me interesa que el jugador 0 mire la 1ra fila.
	pos_actual.second = 1;

	clock_gettime(CLOCK_REALTIME, &tiempo_antes);
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
	clock_gettime(CLOCK_REALTIME, &tiempo_despues);

	
	
	//El mas cercano juega siempre solo.
	bool soy_el_mas_cercano = false;
	if(this->strat == SHORTEST){
		soy_el_mas_cercano = belcebu->soy_el_mas_cercano(nro_jugador,this->equipo);
	}
	if(!soy_el_mas_cercano && this->strat == SHORTEST) return;

	//Idea:   Turno:  | Rojo | Azul | Rojo | Azul | .....
	//	      Ronda       -- 1 --       -- 2 --

	//Estrategias
	while(!this->belcebu->termino_juego()) { // Chequear que no haya una race condition en gameMaster
		switch(this->strat) {
			//SECUENCIAL,RR,SHORTEST,USTEDES

			case(SECUENCIAL): {
				int finalizador = -1;

				//Esperar que toque el turno respectivo
				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo);
				
					//Para que Rojo vuelva jugar despues de su primera vez, debe esperar un cambio de ronda.
					if(belcebu->ronda_actual() > 0){ 
						sem_wait(&belcebu->ronda_anterior_finalizada);
						sem_post(&belcebu->ronda_anterior_finalizada);
					}
                }

                coordenadas coords_bandera = get_pos_bandera_contraria();
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);

                //parte critica: moverse y esperar barrera
                moverse.lock();
				if(!this->belcebu->termino_juego()){
					belcebu->mover_jugador(proxima_dir, nro_jugador);
				}
                cant_jugadores_que_ya_jugaron++;
                if(cant_jugadores_que_ya_jugaron == cant_jugadores){
					finalizador = nro_jugador;

					//Bloquea el semaforo ronda_anterior_finalizada
					if(belcebu->ronda_actual() > 0 && this->equipo == ROJO) sem_post(&belcebu->ronda_anterior_finalizada);
                }
                moverse.unlock();
                //fin parte critica
				
				//Jugaron todos y el thread actual que quiere entrar es el ultimo en jugar.
				//Aca se realiza el cambio de ronda.
				if(cant_jugadores_que_ya_jugaron == cant_jugadores && finalizador == nro_jugador) {
					cant_jugadores_que_ya_jugaron = 0;
					this->belcebu->termino_ronda(this->equipo);
					for (int i = 0; i < cant_jugadores; i++) sem_post(&barrier);
				}
				sem_wait(&barrier);
				
				//Habilita a los jugadores del otro equipo.
				if(this->equipo == AZUL) {
					//El ultimo jugador del equipo azul da como terminada la ronda.
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
				// Se espera a que el jugador i habilite al jugador i+1
				if(equipo == ROJO) {
					sem_wait(&belcebu->mutexes_rr_rojos[nro_jugador]);
				} else{
					sem_wait(&belcebu->mutexes_rr_azules[nro_jugador]);
				}

 				if(quantum_restante > 0) {
					coordenadas coords_bandera = get_pos_bandera_contraria();
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

				//El ultimo jugador habilita al jugador 0.
				if(nro_jugador == finalizador) {
					if(this->equipo == AZUL){
						sem_post(&belcebu->mutexes_rr_rojos[0]);	
					} else {
						sem_post(&belcebu->mutexes_rr_azules[0]);
					}
				}
				break;
			}
			case(SHORTEST) : {
				// por el break de antes del switch, aca corre un solo thread.

				if(this->equipo == AZUL) {
                    sem_wait(&belcebu->turno_azul);
                } else {
                    sem_wait(&belcebu->turno_rojo); 
                }

				//
				if(belcebu->ronda_actual() == 0 && this->equipo == ROJO){
					sem_wait(&belcebu->turno_rojo);
					sem_wait(&belcebu->turno_rojo);
					sem_wait(&belcebu->turno_rojo);
				}

				coordenadas coords_bandera = get_pos_bandera_contraria();
                direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
				belcebu->mover_jugador(proxima_dir, nro_jugador);
				this->belcebu->termino_ronda(this->equipo);

				if(this->equipo == AZUL) {
                    sem_post(&belcebu->turno_rojo);	
                } else { 
                	sem_post(&belcebu->turno_azul);
                }

				break;
			}
			//Le damos a cada jugador una cantidad de movimientos random en cada ronda.
			case(USTEDES): {
				this_thread::sleep_for(100ms);

				// Se espera a que el jugador i habilite al jugador i+1
				if(equipo == ROJO){
					sem_wait(&belcebu->mutexes_rr_rojos[nro_jugador]);
				} else{
					sem_wait(&belcebu->mutexes_rr_azules[nro_jugador]);
				}

 				while(quantums_por_jugador[nro_jugador] > 0) {
					coordenadas coords_bandera = get_pos_bandera_contraria();
                	direccion proxima_dir = apuntar_a(posiciones[nro_jugador], coords_bandera);
					belcebu->mover_jugador(proxima_dir, nro_jugador); // sin mutex, sabemos que esta bloqueado
					quantums_por_jugador[nro_jugador]--;
				} 

				if(nro_jugador == cant_jugadores-1) {
					this->belcebu->termino_ronda(this->equipo);
					reiniciar_quantums();
					if(this->equipo == AZUL){
						sem_post(&belcebu->mutexes_rr_rojos[0]);	
					} else {
						sem_post(&belcebu->mutexes_rr_azules[0]);
					}
				} else {
					if(this->equipo == AZUL) {
						sem_post(&belcebu->mutexes_rr_azules[(nro_jugador + 1) % cant_jugadores]);
					} else {
						sem_post(&belcebu->mutexes_rr_rojos[(nro_jugador + 1) % cant_jugadores]);
					}
				}

				break;
			}
			default:
				break;
		}

		//Actualiza posiciones de los jugadores en el vector de la clase equipo.
		posiciones[nro_jugador] = belcebu->posicion_de(nro_jugador, this->equipo);
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
	this->quantums_por_jugador = vector<int>(cant_jugadores,0);

	//Init mutexes de RR.
	//Inicializados de manera que siempre arranque el jugador numero 0 del equipo.
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

	//Barreras inicializadas en 0
	sem_init(&barrier, 0, 0);
	sem_init(&lejanos, 0, 0);
	
	//TODO: esto se usa todavia?
	for(int i = 0; i < cant_jugadores; i++){
		jugadores_ya_jugaron.push_back(0);
	}

	//Init Quantums random de nuestra implementacion.
	reiniciar_quantums();

	// interpretamos en la consigna que de no alcanzar para cubrir a todos los jugadores con el quantum,
	// todos se mueven una vez.
	if(quantum < cant_jugadores){
		quantum = cant_jugadores;
		quantum_restante = quantum;
	}

    //Iniciar su respectivo semaforo. (De manera de que el rojo juegue primero)
	if(equipo == AZUL){
		sem_init(&belcebu->turno_azul, 0, 0);
	} else {
		sem_init(&belcebu->turno_rojo, 0, cant_jugadores);
	}
}

void Equipo::reiniciar_quantums() {
	int maxQuantum = 10;
	for(int i = 0; i < cant_jugadores; i++){
		int random = rand() % maxQuantum;
		quantums_por_jugador[i] = random;
	}
}

void Equipo::comenzar() {
	
	//clock_gettime(CLOCK_REALTIME, &tiempo_antes);
	//buscar_bandera_naif();
	//clock_gettime(CLOCK_REALTIME, &tiempo_despues);

	// Creamos los jugadores
	for(int i=0; i < cant_jugadores; i++) {
		jugadores.emplace_back(thread(&Equipo::jugador, this, i)); 
	}

}

void Equipo::terminar() {
	for(auto &t : jugadores){
		t.join();
	}

	tiempo_en_encontrar_bandera = tiempo_despues.tv_nsec - tiempo_antes.tv_nsec;
	cout << "Tiempo en encontrar la bandera (nanosec) = " << tiempo_en_encontrar_bandera << endl;

	tiempo_en_encontrar_bandera = tiempo_despues.tv_sec - tiempo_antes.tv_sec;
	cout << "Tiempo en encontrar la bandera (sec) = " << tiempo_en_encontrar_bandera << endl;

}

coordenadas Equipo::get_pos_bandera_contraria() {
	return pos_bandera_contraria;
}

void Equipo::buscar_bandera_naif(){
	
	//mientras exista bandera contraria, esto la encuentra.
	for (size_t x = 1; x < belcebu->getTamx(); x++) {
		
		for (size_t y = 1; y < belcebu->getTamy(); y++) {
			coordenadas temp;
			temp.first = x;
			temp.second = y;
		
			if(belcebu->es_posicion_bandera(temp, this->contrario)){
				bandera_found = true;
				pos_bandera_contraria = temp;
				return;
			}
		}
	}

}