#include <cstdlib>
// utilizada en la generación de números aleatorios para la distribución de repartidores
#include <ctime>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <limits>
#include <climits>

// para configurar consola a UTD-8
// Nota: se realizó de esta manera para evitar errores de compatibilidad con Linux y Mac (utilizo Ubuntu)
#ifdef _WIN32
#include <windows.h>
#endif

// Declaramos límite / infinito para representar que no hay conexión entre dos nodos

using namespace std;

/*
    * Autora: María Martínez V-32.792.066
    * Paradigma: Programación Orientada a Objetos Pura
    * Arquitectura: Modelo Vista Controlador (MVC):
        * Modelo: Sector, Persona, Repartidor, Cliente, Delivery
        * Vista: ClientesCRUD, RepartidoresCRUD, SectoresCRUD, MenuGestion, MenuServicioDiario, MenuPrincipal
        * Controlador: SistemaDelivery;
    * IDE utilizado: CLion
*/

const int INF = INT_MAX;


// Sector (clase para ubicaciones de pedidos / repartidores)
class Sector {
    private:
        std::string id;
        std::string nombre;

    public:
        // constructor por defecto
        Sector() {
            this->id = "";
            this->nombre = "";
        }

        // constructor paramétrico
        Sector(const std::string& id, const std::string& nombre) {
            this->id = id;
            this->nombre = nombre;
        }

        // setters
        void setNombre(const std::string& nombre) {
            this->nombre = nombre;
        }

        // getters
        const std::string& getID() const {
            return this->id;
        }
        const std::string& getNombre() const{
            return this->nombre;
        }

};



class Grafo {
    int **matriz;
    int nSectores;
    std::vector<std::string> ids;

public:
    // Constructor por defecto
    Grafo(): matriz(nullptr), nSectores(0) {};

    // evitamos copias
    Grafo(const Grafo&) = delete;
    Grafo& operator=(const Grafo&) = delete;

    //matriz vacia
    void construir (const std::vector<Sector> sectores) {
        liberar();

        nSectores = static_cast<int>(sectores.size());
        // ajustar tamaño de la lista dinamica para que sea el mismo que el vector de sectores
        ids.resize(nSectores);

        for (int i = 0; i < nSectores; i++) {
            ids[i] = sectores[i].getID();
        }

        matriz = new int*[nSectores];
        for (int i = 0; i < nSectores; i++) {
            matriz[i] = new int[nSectores];
            for (int j = 0; j < nSectores; j++) {
                matriz[i][j] = (i==j) ? 0 : INF;
            }
        }
    }

    int obtenerIndice(const std::string &id) const {
        for (int i=0;i<nSectores;i++) {
            if (ids[i] == id) {
                return i;
            }
        }
        return -1;
    }

    void cargarSectores(const std::string& archivo) {
        if (matriz == nullptr) {
            cout << "Debe construir el grafo antes de cargar datos :("<<endl;
            return;
        }

        std::ifstream arc(archivo);
        if (!arc.is_open()) {
            cout << "Error al abrir el archivo. El grafo no tendrá conexiones :("<<endl;
            return;
        }

        std::string linea;
        int nLinea = 0;
        while (getline(arc, linea)) {
            nLinea ++;

            if (linea.empty()) {
                continue;
            }

            std::stringstream ss(linea);
            std::string origenID, destinoID, distanciaStr;

            getline(ss, origenID, '|');
            getline(ss, destinoID, '|');
            getline(ss, distanciaStr, '|');


            if (origenID.empty() || destinoID.empty() || distanciaStr.empty()) {
                cout << "La línea " << nLinea << " está mal formada, se omitirá en el grafo... :("<<endl;
                continue;
            }

            int distancia;
            try {
                // convertimos la distancia a un
                distancia = std::stoi(distanciaStr);
            } catch (...) {
                cout << "Distancia inválida en línea [" << nLinea << "], se omitirá en el grafo." << endl;
                continue;
            }

            // forzamos la distancia de 1 para que no se confunda con la diagonal 0
            if (distancia < 1) {
                distancia = 1;
            }

            int i = obtenerIndice(origenID);
            int j = obtenerIndice(destinoID);

            if (i == -1 || j == -1) {
                cout << "Sector no reconocido en línea " << nLinea << " [" << linea << "], se omitirá del grafo :(" << endl;
                continue;
            }

            // como el grafo es no-dirigido -> reflejo la distancia en ambos nodos del grafo
            matriz[i][j]=distancia;
            matriz[j][i]=distancia;
        }

        // cerramos el archivo
        arc.close();
    }

    std::pair<std::vector<std::string>,int> aplicarDijkstra(const std::string& origenID, const std::string& destinoID) const {
        int inicio = obtenerIndice(origenID);
        int fin = obtenerIndice(destinoID);

        if (inicio == -1 || fin == -1 || nSectores == 0) {
            return {{}, -1};
        }

        std::vector<int> dist(nSectores, INF); // vector de distancias
        std::vector<int> prev(nSectores, -1); // sectores previos
        std::vector<bool> visitado(nSectores, false); // vector con el estado de los sectores (visitado o no visitado)
        dist[inicio] = 0; // iniciamos etiquetando el nodo fuente del grafo con el ID de la direccion de inicio y dandole distancia de 0

        for (int i = 0; i < nSectores; i++) {
            int u= -1;
            int distanciaMin = INF;

            for (int j=0;j<nSectores;j++) {
                if (!visitado[j] && dist[j] < distanciaMin) {
                    distanciaMin = dist[j];
                    u=j;
                }
            }

            // el nodo que queda es inalcanzables
            if (u == -1) break;
            visitado[u] = true;

            for (int j=0;j<nSectores;j++) {
                if (!visitado[j] && matriz[u][j] != INF && dist[u] + matriz[u][j] < dist[j]) {
                    dist[j] = dist[u] + matriz[u][j];
                    prev[j] = u;
                }
            }
        }

        // si se ha llegado al final -> no existe más ruta
        if (dist[fin] == INF) {
            return {{}, -1};
        }

        std::vector<std::string> camino;
        int actual = fin;
        while (actual != -1) {
            camino.push_back(ids[actual]);
            actual = prev[actual];

        }
        std::reverse(camino.begin(), camino.end());
        return {camino, dist[fin]};
    }

    // liberar memoria si la matriz ya existia
    void liberar() {
        if (matriz != nullptr) {
            for (int i = 0; i < nSectores; i++) {
                delete[] matriz[i];
            }
            delete[] matriz;
            matriz = nullptr;
        }
        nSectores = 0;
        ids.clear();
    }

    ~Grafo(){
        liberar();
    }
};

// Clase Padre para personas de la que heredan: Repartidor y Cliente
class Persona {
    private:
        std::string cedula;
        std::string nombre;

    public:
        // Constructor por defecto
        Persona() = default;

        // Constructor paramétrico
        Persona(const std::string& cedula,const std::string& nombre){
            this->cedula=cedula;
            this->nombre = nombre;
        }

        // setter
        void setNombre(const std::string& nombre) {
            this->nombre = nombre;
        }

        // getters
        const std::string& getCedula() const{
            return this->cedula;
        }
        const std::string& getNombre() const{
            return this->nombre;
        }

        // habilitar polimorfismo en tiempo de ejecución
        // (así Repartidor y Cliente pueden heredar de persona y hacer override a los métodos de ser necesario)
        virtual ~Persona() {}
};

class Repartidor: public Persona {
    private:
        std::string placa;
        std::string modelo;
        int numEntregas;
        bool disponible;
        Sector ubicacion;
    public:

    Repartidor(): Persona() {
        this->placa = "";
        this->modelo = "";
        this->disponible = false;
        this->numEntregas= 0;
    }

    Repartidor(const std::string& cedula, const std::string& nombre, const Sector& ubicacion,const  std::string& placa, const std::string& modelo, int numEntregas, bool disponible) :
    Persona(cedula, nombre) {
        this->placa = placa;
        this->modelo = modelo;
        this->disponible = disponible;
        this->numEntregas=numEntregas;
        this->ubicacion = ubicacion;
    }

    // setters
    void setPlaca(const std::string& placa) {
        this->placa= placa;
    }
    void setModelo(const std::string& modelo) {
        this->modelo= modelo;
    }
    void setDisponible(bool disponible) {
        this->disponible = disponible;
    }
    void setNumEntregas(int numEntregas) {
        this->numEntregas=numEntregas;
    }
    void setUbicacion(const Sector& ubicacion) {
        this->ubicacion= ubicacion;
    }

    //getters
    const std::string& getPlaca() const{
        return this->placa;
    }
    const std::string& getModelo() const{
        return this->modelo;
    }
    bool isDisponible() const{
        return this->disponible;
    }
    int getNumEntregas() const{
        return this->numEntregas;
    }
    const Sector& getUbicacion() const{
        return this->ubicacion;
    }

};

class Cliente: public Persona {
    private:
        std::string telefono;
        int numPedidos;
    public:
        //
        Cliente(): Persona() {
            this->telefono = "";
            this->numPedidos= 0;
        }
            Cliente(const std::string& cedula,const std::string& nombre, const std::string& telefono, int numPedidos) :
            Persona(cedula,nombre) {
            this->telefono=telefono;
            this->numPedidos=numPedidos;
        }

        // setters
        void setTelefono(const std::string& telefono) {
            this->telefono=telefono;
        }
        void setNumPedidos(int numPedidos){
            this->numPedidos= numPedidos;
        }

        // getters
        const std::string& getTelefono() const{
            return this->telefono;
        }
        int getNumPedidos() const {
            return this->numPedidos;
        }

};

class Delivery {
    private:
        std::string clienteID;
        std::string repartidorID;
        Sector origen;
        Sector destino;
        bool completado;

    public:
        Delivery() = default;

        Delivery(const std::string& clienteID, const std::string& repartidorID, const Sector& origen, const Sector& destino, bool completado) {
            this->clienteID = clienteID;
            this->repartidorID = repartidorID;
            this->origen = origen;
            this->destino = destino;
            this->completado = completado;
        }

        // setters
        void setClienteID(const std::string& clienteID) {
            this->clienteID = clienteID;
        }
        void setRepartidorID(const std::string& repartidorID) {
            this->repartidorID = repartidorID;
        }
        void setOrigen(const Sector& origen) {
            this->origen = origen;
        }
        void setDestino(const Sector& destino) {
            this->destino = destino;
        }
        void setCompletado(bool completado) {
            this->completado = completado;
        }

        // getters
        const std::string& getClienteID() const {
            return clienteID;
        }
        const std::string& getRepartidorID() const {
            return repartidorID;
        }
        const Sector& getOrigen() const {
            return origen;
        }
        const Sector& getDestino() const {
            return destino;
        }
        bool isCompletado() const {
            return completado;
        }
};

int leerOpcion() {
    int opcion;

    while (true) {
        cin >> opcion;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Entrada inválida. Ingrese un número." << endl;
            continue; // vuelve a pedir la opción
        }

        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return opcion;
    }
}

// Controlador del sistema
class SistemaDelivery {

    private:
        // arrays dinámicos que contendrán los datos registrados
        std::vector<Sector> sectores;
        std::vector<Repartidor> repartidores;
        std::vector <Cliente> clientes;
        std::vector <Delivery> deliveries;
        Grafo grafo;

    public:

    // constructor por defecto
    SistemaDelivery() = default;

    // constructor paramétrico
    SistemaDelivery(const std::vector<Sector>& sectores, const std::vector<Repartidor>& repartidores, const std::vector<Cliente>& clientes, const std::vector<Delivery> deliveries) {
        this->sectores = sectores;
        this->repartidores = repartidores;
        this->clientes = clientes;
        this->deliveries = deliveries;
    }

    // Para cargar el grafo
    void cargarGrafo() {
        grafo.construir(sectores);
        grafo.cargarSectores("Grafo.txt");
        cout << "Grafo cargado con éxito."<<endl;
    }

    // getter para el grafo
    const Grafo& getGrafo() const {
        return this->grafo;
    }

    // Búsqueda general por ID -> de retornar nullptr, el objeto buscado no existe

    Sector* buscarSector(const std::string& id) {
        for (Sector& s : this->sectores) {
            if (s.getID() == id) {
                return &s;
            }
        }
        return nullptr;
    };

    Repartidor* buscarRepartidor(const std::string& id) {
        for (Repartidor& r: this->repartidores) {
            if (r.getCedula() == id || r.getPlaca() == id) {
                return &r;
            }
        }
        return nullptr;
    }

    Cliente* buscarCliente(const std::string& id) {
        for (Cliente& c: this->clientes) {
            if (c.getCedula() == id) {
                return &c;
            }
        }
        return nullptr;
    }

    // Funcionalidad # 1: Gestión (CRUD) de clientes

    void agregarCliente() {
        std::string cedula, nombre, telefono;

        bool existe;
        do {
            existe = false;

            cout<<"Cédula: ";
            do {
                getline(std::cin, cedula);
            } while (cedula == "");

            for (const  Cliente& c: this->clientes) {
                if (c.getCedula() == cedula) {
                    existe = true;
                    cout<<"Ya existe un cliente con ese número de cedula."<<endl;
                    break;
                }
            }
        } while (existe);

        cout << "Nombre: "<<endl;
        do {
            std::getline(std::cin, nombre);
        } while (nombre=="");

        cout << "Número de telefono: "<<endl;
        do {
            std::getline(std::cin, telefono);
        } while (telefono=="");

        Cliente c(cedula, nombre, telefono,0);

        clientes.push_back(c);
        cout << "Cliente registrado exitosamente."<<endl;
    }

    void modificarCliente() {
        std::string cedula;

        cout << "Cedula: "<<endl;
        do {
            std::getline(std::cin, cedula);
        } while (cedula == "");

        int opcion;

        Cliente* cliente = buscarCliente(cedula);
        if (cliente==nullptr) {
            cout<< "Cliente no encontrado."<<endl;
            return;
        }

        do {
            cout << "OPCIONES "<<endl;
            cout << "1. Nombre" <<endl;
            cout << "2. Telefono" <<endl;
            cout << "Ingrese la opción que desea modificar: ";

            opcion = leerOpcion();

        } while (opcion<1 || opcion>2);

        switch (opcion) {
            case 1: {
                std::string nombre;

                cout << "Nombre: "<<endl;
                do {
                    std::getline(std::cin, nombre);
                } while (nombre=="");

                (*cliente).setNombre(nombre);

                cout << "Nombre modificado con éxito"<<endl;
                break;
            }

            case 2: {
                std::string telefono;
                cout << "Nuevo número telefónico: "<<endl;
                do {
                    std::getline(std::cin, telefono);
                } while (telefono=="");

                (*cliente).setTelefono(telefono);

                cout << "Nombre modificado con éxito"<<endl;
                break;
            }
        }
    }

    void eliminarCliente() {
        std::string cedula;
        cout<<"ID: ";
        do {
            getline(std::cin, cedula);
        } while (cedula == "");

        Cliente* cliente = buscarCliente(cedula);

        if (cliente==nullptr) {
            cout<< "Cliente no encontrado."<<endl;
            return;
        }

        auto pos = clientes.begin() + (cliente - clientes.data());
        clientes.erase(pos);

        cout<< "Cliente eliminado con éxito."<<endl;
    }

    void consultarCliente() {
        std::string cedula;
        cout<<"ID: ";
        do {
            getline(std::cin, cedula);
        } while (cedula == "");

        Cliente* cliente = buscarCliente(cedula);

        if (cliente==nullptr) {
            cout<< "Cliente no encontrado."<<endl;
            return;
        }

        cout << "Cliente encontrado:"<<endl;
        cout << "Cédula: " << cliente->getCedula() << endl;
        cout << "Nombre: " << cliente->getNombre() << endl;
        cout << "Telefono: " << cliente->getTelefono() << endl;
        cout << "Número de pedidos realizados: "<<cliente->getNumPedidos()<<endl;

        return;
    }

    // Funcionalidad # 2: Gestionar CRUD repartidores

    void agregarRepartidor() {
        std::string cedula, nombre, modelo, placa, sectorID;
        Sector* sector;

        bool existe;
        do {
            existe = false;

            cout<<"Cédula: ";
            do {
                getline(std::cin, cedula);
            } while (cedula == "");

            for (const  Repartidor& r: this->repartidores) {
                if (r.getCedula() == cedula) {
                    existe = true;
                    cout<<"Ya existe un repartidor con ese número de cedula."<<endl;
                    break;
                }
            }
        } while (existe);


        cout << "Nombre: ";
        do {
            std::getline(std::cin, nombre);
        } while (nombre=="");

        cout<< "Modelo de vehículo: ";
        do {
            std::getline(std::cin, modelo);
        } while (modelo=="");

        do {
            existe = false;

            cout<< "Placa: ";
            do {
                std::getline(std::cin, placa);
            } while (placa=="");


            for (const  Repartidor& r: this->repartidores) {
                if (r.getPlaca() == placa) {
                    existe = true;
                    cout<<"Ya existe un repartidor con esa placa de vehículo."<<endl;
                    break;
                }
            }
        } while (existe);

        do {
            cout << "ID del sector: ";
            do {
                std::getline(std::cin, sectorID);
            } while (sectorID=="");

            sector = buscarSector(sectorID);

            if (sector==nullptr) {
                cout<< "Sector no encontrado."<<endl;
            }
        } while (sector==nullptr);

        Repartidor r(cedula,nombre,*sector,placa,modelo,0,true);
        repartidores.push_back(r);
        cout<< "Repartidor registrado con éxito."<<endl;
    }

    void modificarRepartidor() {

        int opcion;

        std::string id;
        cout << "Ingrese cedula o placa del repartidor: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Repartidor* r = buscarRepartidor(id);

        if (r==nullptr) {
            cout<< "Repartidor no encontrado."<<endl;
            return;
        }

        do {
            cout << "OPCIONES "<<endl;
            cout << "1. Nombre" <<endl;
            cout << "2. Modelo de vehículo"<<endl;
            cout << "3. Placa de vehiculo"<<endl;
            cout << "4. Disponibilidad" <<endl;
            cout << "Ingrese la opción que desea modificar: ";
            opcion = leerOpcion();

        } while (opcion<1 || opcion>4);

        switch (opcion) {
            case 1: {
                std::string nombre;

                cout << "Nuevo nombre: "<<endl;
                do {
                    std::getline(std::cin, nombre);
                } while (nombre=="");

                (*r).setNombre(nombre);

                cout << "Nombre modificado con éxito"<<endl;
                break;
            }

            case 2: {
                std::string modelo;
                cout << "Nuevo modelo de vehículo: "<<endl;
                do {
                    getline(std::cin, modelo);
                } while (modelo=="");

                (*r).setModelo(modelo);

                cout << "Modificación exitosa."<<endl;
                break;
            }
            case 3: {
                std::string placa;
                cout << "Nueva placa de vehículo: "<<endl;
                do {
                    getline(std::cin, placa);
                } while (placa=="");

                (*r).setPlaca(placa);

                cout << "Modificación exitosa."<<endl;
                break;
            }
            case 4: {
                std::string res;
                cout << "Estado actual: "<< (r->isDisponible() ? "Disponible" : "Ocupado") << endl;

                do {
                    cout << "¿Desea modificarlo?: (S/N)";
                    do {
                        getline(std::cin, res);
                    } while (res=="");

                    for (char &c : res) {
                        c = toupper(c);
                    }

                } while (res != "S" && res != "N");

                if (res == "N") {
                    return;
                } else {
                    r->setDisponible(!r->isDisponible());
                }
            }
        }
    }

    void eliminarRepartidor() {
        std::string id;

        cout << "Ingrese cedula o placa del repartidor: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Repartidor* r = buscarRepartidor(id);

        if (r==nullptr) {
            cout<< "Repartidor no encontrado."<<endl;
            return;
        }

        auto pos = repartidores.begin() + (r - repartidores.data());
        repartidores.erase(pos);
        cout <<"Repartidor eliminado con éxito."<<endl;
    }

    void consultarRepartidor() {
        std::string id;

        cout << "Ingrese cedula o placa del repartidor: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Repartidor* r = buscarRepartidor(id);

        if (r==nullptr) {
            cout<< "Repartidor no encontrado."<<endl;
            return;
        }

        cout << "Repartidor encontrado. "<<endl;
        cout << "Nombre: " << r->getNombre() << endl;
        cout << "Cedula: " << r->getCedula() << endl;
        cout << "Placa: " << r->getPlaca() << endl;
        cout << "Modelo del vehículo: " << r->getModelo() << endl;
        cout << "Ubicación actual: " << r->getUbicacion().getNombre() << endl;
        cout << "N° Entregas realizadas: " << r->getNumEntregas() << endl;
    }

    // Funcionalidad # 3: Gestión CRUD de sectores

    void agregarSector() {
        std::string id;
        std::string nombre;
        bool existe;
        do {
            existe = false;

            cout << "ID: "<<endl;
            do {
                std::getline(std::cin, id);
            } while (id=="");

            cout << "Nombre: "<<endl;
            do {
                std::getline(std::cin, nombre);
            } while (nombre=="");

            for (const  Sector& s: this->sectores) {
                if (s.getID() == id) {
                    existe = true;
                    cout<<"ID ocupado."<<endl;
                    break;
                }
                if (s.getNombre() == nombre) {
                    existe = true;
                    cout<<"Sector ya existente."<<endl;
                    break;
                }
            }
        } while (existe);

        Sector s(id,nombre);
        sectores.push_back(s);
    }

    void modificarSector() {
        std::string nombre;
        std::string id;
        cout << "ID: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Sector* sector = buscarSector(id);
        if (sector==nullptr) {
            cout<< "Sector no encontrado."<<endl;
            return;
        }

        cout<< "Ingrese nuevo nombre del sector: "<<endl;
        do {
            std::getline(std::cin, nombre);
        } while (nombre=="");

        sector->setNombre(nombre);
        cout<<"Modificación exitosa"<<endl;
    }

    void eliminarSector() {
        std::string id;
        cout << "ID: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Sector* sector = buscarSector(id);
        if (sector==nullptr) {
            cout<< "Sector no encontrado."<<endl;
            return;
        }

        for (const Repartidor& r: repartidores) {
            if (r.getUbicacion().getID() == sector->getID()) {
                cout<<"No se puede eliminar. Hay repartidores asignados."<<endl;
                return;
            }
        }

        // Determinar índice del objeto en el vector
        auto pos = sectores.begin() + (sector - sectores.data());
        sectores.erase(pos);

        cout << "Sector eliminado con éxito"<<endl;
        return;
    }

    void consultarSector() {
        std::string id;
        cout << "ID: "<<endl;
        do {
            std::getline(std::cin, id);
        } while (id=="");

        Sector* sector = buscarSector(id);
        if (sector==nullptr) {
            cout<< "Sector no encontrado."<<endl;
            return;
        }

        cout << "Sector encontrado:"<<endl;
        cout << "ID: "<<sector->getID()<<endl;
        cout << "Nombre: "<<sector->getNombre()<<endl;
    }

    // Funcionalidad # 4: Actualizar ubicación del repartidor

    void actualizarUbicacionRepartidor() {
        std::string id;

        cout << "Ingrese cedula o placa del repartidor: "<<endl;
        cin >> id;

        Repartidor* r = buscarRepartidor(id);

        if (r==nullptr) {
            cout<< "Repartidor no encontrado."<<endl;
            return;
        }

        std::string idSector;
        cout << "Ingrese ID del nuevo sector: "<<endl;
        cin >> idSector;

        Sector* sector = buscarSector(idSector);
        if (sector==nullptr) {
            cout<< "Sector no encontrado."<<endl;
        } else {
            (*r).setUbicacion(*sector);
            cout << "Modificación exitosa."<<endl;
        }
    }

    void distribuirRepartidores() {

        if (sectores.empty()) {
            cout<< "No hay sectores registrados"<<endl;
            return;
        }

        if (repartidores.empty()) {
            cout<< "No hay repartidores registrados"<<endl;
            return;
        }

        for (Repartidor& r: repartidores) {
            int randSector = rand() % sectores.size();
            r.setUbicacion(sectores[randSector]);
        }

        cout<< "Repartidores distribuidos con éxito."<<endl;
    }

    void solicitarDelivery() {
        std::string cedula;
        std::string origenID;
        std::string destinoID;

        cout<<"Ingrese número de cédula: ";
        cin >> cedula;

        Cliente* cliente = buscarCliente(cedula);
        if (cliente==nullptr) {
            cout<< "Cliente no encontrado. Debe registrarse primero"<<endl;
            return;
        }

        cout<< "Ingrese ID del sector de origen: ";
        cin >> origenID;
        Sector* origen = buscarSector(origenID);
        if (origen==nullptr) {
            cout<< "Sector no encontrado."<<endl;
            return;
        }

        cout<< "Ingrese ID del sector de destino: ";
        cin >> destinoID;
        Sector* destino = buscarSector(destinoID);
        if (destino==nullptr) {
            cout<< "Sector no encontrado."<<endl;
            return;
        }

        seleccionarRepartidor(*cliente,*origen,*destino);
    }

    // retorna vector con los repartidores disponibles en un sector
    std::vector<Repartidor*> verRepartidoresDisponibles(const Sector& sector) {
        std::vector<Repartidor*> rDisponibles;
        for (Repartidor& r: repartidores) {
            if (r.isDisponible()) {
                rDisponibles.push_back(&r);
            }
        }
        return rDisponibles;
    }

    void seleccionarRepartidor(Cliente& cliente, const Sector& origen, const Sector& destino) {

        vector<Repartidor*> rDisponibles = verRepartidoresDisponibles(origen);

        if (rDisponibles.empty()) {
            cout<< "No hay repartidores disponibles en el sector. Intente más tarde."<<endl;
            return;
        }

        cout<< "Repartidores disponibles: "<<endl;
        cout<< "\t" << "NOMBRE" << "\t" << "MODELO VEHÍCULO" << "\t" << "PLACA" << endl;

        for (size_t i = 0; i < rDisponibles.size(); i++) {
            cout << i+1 << ".\t"<< rDisponibles[i]->getNombre() << "\t" << rDisponibles[i]->getModelo()<< "\t" << rDisponibles[i]->getPlaca() << endl;
        }
        int res;

        do {
            cout << "Elija un repartidor: ";
            cin >> res;

            if (cin.fail()) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Entrada inválida. Ingrese un número." << endl;
                continue;
            }
        } while (res<1 || res> static_cast<int>(rDisponibles.size()));

        Repartidor* rSeleccionado = rDisponibles[res-1];

        // Calcular y mostrar la ruta mínima
        auto caminoMin = grafo.aplicarDijkstra(rSeleccionado->getUbicacion().getID(),origen.getID());
        std::vector<std::string> ruta = caminoMin.first;
        int distTotal = caminoMin.second;
        if (distTotal == -1) {
            cout << "No existe una ruta disponible. Verifique las conexiones del grafo."<<endl;
            return;
        }

        cout << "\nRuta mínima hacia el cliente:"<<endl;
        for (size_t i = 0; i < ruta.size(); i++) {
            Sector* s = buscarSector(ruta[i]);
            cout << (s != nullptr ? s->getNombre() : ruta[i]);
            if (i != ruta.size() - 1) cout << " -> ";
        }
        cout << endl;

        cout << "Distancia total del recorrido: " << distTotal << " km" << endl;

        rSeleccionado->setDisponible(false);

        Delivery d(cliente.getCedula(),rSeleccionado->getCedula(),origen,destino,false);
        deliveries.push_back(d);

        cliente.setNumPedidos(cliente.getNumPedidos()+1);
    }

    void generarReporte() {
        std::ofstream reporte ("reporte_estadisticas.txt");

        if (!reporte.is_open()) {
            cout << "Error al abrir/crear archivo"<<endl;
        } else {
            reporte << "ESTADISTICAS DE LA JORNADA - SPEED DELIVERY" << endl;

            // generar vector de repartidores en orden descendente según el número de entregas
            std::vector<Repartidor>rOrdenados = repartidores;

            std::sort(rOrdenados.begin(), rOrdenados.end(), [](const Repartidor& r1, const Repartidor& r2) {
                return r1.getNumEntregas() > r2.getNumEntregas();
            });

            // generar vector de clientes en orden descendente según el número de pedidos que han realizado
            std::vector<Cliente>cOrdenados = clientes;

            std::sort(cOrdenados.begin(), cOrdenados.end(), [](const Cliente& c1, const Cliente& c2) {
                return c1.getNumPedidos() > c2.getNumPedidos();
            });

            reporte << left << "\n\nREPARTIDORES MÁS SELECCIONADOS:"<<endl;
            reporte << left << "|" << setfill('-') << setw(63) << right << "|" << endl ;
            reporte << left << setfill(' ') << "|" << setw(16) << "CÉDULA" << "|" << setw(30) << "NOMBRE"  << "|" << setw(16) << "N° ENTREGAS" << "|" << endl;
            reporte << left << "|" << setfill('-') << setw(63) << right << "|" << endl ;
            for (Repartidor& r: rOrdenados) {
                reporte << "|" << setfill(' ') << setw(15) << r.getCedula() << "|" << setw(30) << r.getNombre() << "|" << setw(15) << r.getNumEntregas() << "|" << endl;
            }
            reporte << "|" << setfill('-') << setw(63) << right << "|" << endl ;

            reporte << left << "\n\nCLIENTES FRECUENTES:"<<endl;
            reporte << left << "|" << setfill('-') << setw(63) << right << "|" << endl ;
            reporte << left << setfill(' ') << "|" << setw(16) << "CÉDULA" << "|" << setw(30) << "NOMBRE"  << "|" << setw(16) << "N° PEDIDOS" << "|" << endl;
            reporte << left << "|" << setfill('-') << setw(63) << right << "|" << endl ;
            for (Cliente& c: cOrdenados) {
                reporte << left << "|" << setfill(' ') << setw(15) << c.getCedula() << "|" << setw(30) << c.getNombre() << "|" << setw(15) << c.getNumPedidos() << "|" << endl;
            }
            reporte << left << "|" << setfill('-') << setw(63) << right << "|" << endl ;

            cout << "Reporte generado con éxito" << endl;
        }
        reporte.close();
    }

    void finalizarEntrega() {
        bool existe = false;
        std::string id;
        cout<<"Ingrese placa del vehículo o ID del repartidor: ";
        cin >> id;

        Repartidor* r = buscarRepartidor(id);

        if (r==nullptr) {
            cout<< "Repartidor no encontrado."<<endl;
            return;
        }


        Sector destino;
        for (Delivery& d: deliveries ) {
            if (d.getRepartidorID() == r->getCedula() && !d.isCompletado()) {
                d.setCompletado(true);
                destino = d.getDestino();
                existe = true;
                break;
            }
        }

        if (!existe) {
            cout << "El repartidor no tiene entregas asignadas."<<endl;
            return;
        }

        r->setDisponible(true);
        r->setUbicacion(destino);
        r->setNumEntregas(r->getNumEntregas()+1);
        cout<< "La entrega ha sido completada con éxito."<<endl;
    }

    // PERSISTENCIA DE DATOS

    void guardarClientes() {
        ofstream arc("clientes.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir/crear archivo" <<endl;
            return;
        }

        for (Cliente& c: clientes) {
            arc <<c.getCedula() << "|" <<  c.getNombre() << "|" << c.getTelefono() << "|"<< c.getNumPedidos() << endl;
        }
        arc.close();
    }

    void cargarClientes() {
        ifstream arc("clientes.txt");

        if (!arc.is_open()) {

            cout << "Error al abrir archivo" <<endl;
            return;
        }

        string linea;

        while (getline(arc, linea)) {
            stringstream ss(linea);
            string parte;
            std::string array[4];

            int i = 0;
            while (getline(ss, parte, '|')) {
                array[i] = parte;
                i++;
            }

            // Convertir la cadena de texto que contiene el número de pedidos a un entero
            int numPedidos = std::stoi(array[3]);
            Cliente c(array[0],array[1],array[2],numPedidos);
            clientes.push_back(c);
        }
    }

    void guardarRepartidores() {

        ofstream arc("repartidores.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir/crear archivo" <<endl;
            return;
        }

        for (Repartidor& r: repartidores) {
            arc << r.getCedula()<< "|" << r.getNombre() << "|" << r.getUbicacion().getID() << "|" << r.getPlaca() << "|" << r.getModelo() << "|" << r.getNumEntregas() << "|" << (r.isDisponible() ? "Disponible" : "Ocupado")<< endl;
        }
        arc.close();
    }


    void cargarRepartidores() {
        ifstream arc("repartidores.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir archivo" <<endl;
            return;
        }

        string linea;

        while (getline(arc, linea)) {
            stringstream ss(linea);
            string parte;
            std::string array[7];

            int i = 0;
            while (getline(ss, parte, '|')) {
                array[i] = parte;
                i++;
            }

            // Convertir la cadena de texto que contiene el número de pedidos a un entero
            int numEntregas = std::stoi(array[5]);

            bool disponible;
            if (array[6] == "Disponible") {
                disponible = true;
            } else {
                disponible = false;
            }

            Sector* sector = buscarSector(array[2]);
            if (sector == nullptr) {
                continue; // saltar esta línea
            }

            Repartidor r(array[0],array[1],*sector,array[3],array[4],numEntregas,disponible);
            repartidores.push_back(r);
        }
    }

    void guardarSectores() {
        ofstream arc("sectores.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir/crear archivo" <<endl;
            return;
        }

        for (Sector& s: sectores) {
            arc << s.getID() << "|" << s.getNombre() << endl;
        }
        arc.close();
    }

    void cargarSectores() {
        ifstream arc("sectores.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir archivo" <<endl;
            return;
        }

        string linea;

        while (getline(arc, linea)) {
            stringstream ss(linea);
            string parte;
            std::string array[2];

            int i = 0;
            while (getline(ss, parte, '|')) {
                array[i] = parte;
                i++;
            }
            Sector s(array[0],array[1]);
            sectores.push_back(s);
        }
        arc.close();
    }

    void guardarDeliveries() {
        ofstream arc("deliveries.txt");

        if (!arc.is_open()) {
            cout << "Error al abrir/crear archivo" <<endl;
            return;
        }

        for (Delivery& d: deliveries) {
            arc << d.getClienteID() << "|" << d.getRepartidorID() << "|" << d.getOrigen().getID() <<"|"<< d.getDestino().getID() << "|"<<(d.isCompletado() ? 1 : 0) <<endl;
        }
        arc.close();
    }

    void cargarDeliveries() {
        ifstream arc("deliveries.txt");
        if (!arc.is_open()) {
            cout << "Error al abrir archivo" <<endl;
            return;
        }

        string linea;
        while (getline(arc, linea)) {
            stringstream ss(linea);
            string parte;
            std::string array[5];
            int i = 0;
            while (getline(ss, parte, '|')) {
                array[i] = parte;
                i++;
            }

            Sector* origen = buscarSector(array[2]);
            if (origen == nullptr) {
                cout << "Error en el sector" <<endl;
                continue;
            }

            Sector* destino= buscarSector(array[3]);
            if (destino == nullptr) {
                cout << "Error en el sector" <<endl;
                continue;
            }

            bool completado = false;
            if (array[4] == "1") {
                completado = true;
            } else {
                completado = false;
            }

            Delivery d(array[0],array[1],*origen,*destino,completado);
            deliveries.push_back(d);
        }
    }

    bool sinDatos() const {
        return sectores.empty() && repartidores.empty() && clientes.empty();
    }

    void cargarDatosEjemplo() {

        // Sectores
        sectores.push_back(Sector("S1", "Barrio Obrero"));
        sectores.push_back(Sector("S2", "Pirineos"));
        sectores.push_back(Sector("S3", "Centro"));
        sectores.push_back(Sector("S4", "La Concordia"));
        sectores.push_back(Sector("S5", "Pueblo Nuevo"));

        // Repartidores
        repartidores.push_back(Repartidor("V1111111", "Carlos Ramirez", sectores[0], "ABC123", "Yamaha YBR", 0, true));
        repartidores.push_back(Repartidor("V2222222", "Luis Mendoza",sectores[1], "DEF456", "Suzuki AX100", 0, true));
        repartidores.push_back(Repartidor("V3333333", "Pedro Gomez",sectores[2], "GHI789", "Honda CG150", 0, true));
        repartidores.push_back(Repartidor("V4444444", "Andres Rojas",sectores[3], "JKL012", "Bera BR150", 0, true));
        repartidores.push_back(Repartidor("V5555555", "Tom Holland",sectores[4], "MNO345", "Yamaha Crypton", 0, true));
        repartidores.push_back(Repartidor("V6666666", "Jose Duran",sectores[0],"PQR678", "Empire Keeway", 0, true));
        repartidores.push_back(Repartidor("V7777777", "Ricardo Paez",sectores[1], "STU901", "Suzuki GN125", 0, true));
        repartidores.push_back(Repartidor("V8888888", "Daniel Sanchez",sectores[2], "VWX234", "Honda XR150", 0, true));

        // Clientes
        clientes.push_back(Cliente("V9999999", "Maria Martinez","04121234567", 0));
        clientes.push_back(Cliente("V1010101", "Taylor Swift","04147654321", 0));
        clientes.push_back(Cliente("V1212121", "Camila Carlosama","04248889999", 0));

        cout << "Datos de ejemplo precargados: " << sectores.size() << " sectores, " << repartidores.size() << " repartidores, " << clientes.size() << " clientes." << endl;
    }
};


// MENÚS

void menuServicioDiario(SistemaDelivery& sistema) {
    int opcion;
    sistema.distribuirRepartidores();

    do {

        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << "SPEED DELIVERY - SERVICIO DIARIO" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(40) << left << " 1. Actualizar ubicación de repartidor" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(40) << left << " 2. Solicitar envío" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Finalizar entrega" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 4. Volver" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;

        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                sistema.actualizarUbicacionRepartidor();
                break;
            } case 2: {
                sistema.solicitarDelivery();
                break;
            } case 3: {
                sistema.finalizarEntrega();
                break;
            } case 4: {
                break;
            } default:{
                cout<< "Opcion invalida"<<endl;
                break;
            }
        }

    } while (opcion != 4);
}

void clientesCRUD(SistemaDelivery& sistema) {

    int opcion;
    do {
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << "SPEED DELIVERY - CLIENTES" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 1. Agregar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 2. Modificar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Consultar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 4. Eliminar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 5. Volver" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;

        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                sistema.agregarCliente();
                break;
            } case 2: {
                sistema.modificarCliente();
                break;
            } case 3: {
                sistema.consultarCliente();
                break;
            } case 4: {
                sistema.eliminarCliente();
                break;
            } case 5: {
                break;
            } default: {
                cout<< "Opcion invalida"<<endl;
                break;
            }
        }
    } while (opcion != 5);
}

void sectoresCRUD(SistemaDelivery& sistema) {

    int opcion;
    do {
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << "SPEED DELIVERY - SECTORES" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 1. Agregar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 2. Modificar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Consultar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 4. Eliminar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 5. Volver" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                sistema.agregarSector();
                break;
            } case 2: {
                sistema.modificarSector();
                break;
            } case 3: {
                sistema.consultarSector();
                break;
            } case 4: {
                sistema.eliminarSector();
                break;
            } case 5: {
                break;
            } default: {
                cout<< "Opcion invalida"<<endl;
                break;
            }
        }

    } while (opcion != 5);
}

void repartidoresCRUD(SistemaDelivery& sistema) {

    int opcion;
    do {

        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << "\tSPEED DELIVERY - REPARTIDORES" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 1. Agregar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 2. Modificar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Consultar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 4. Eliminar" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 5. Volver" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;

        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                sistema.agregarRepartidor();
                break;
            } case 2: {
                sistema.modificarRepartidor();
                break;
            } case 3: {
                sistema.consultarRepartidor();
                break;
            } case 4: {
                sistema.eliminarRepartidor();
                break;
            } case 5: {
                break;
            } default: {
                cout<< "Opcion invalida"<<endl;
                break;
            }
        }

    } while (opcion != 5);

}

void menuGestion(SistemaDelivery& sistema) {
    int opcion;
    do {
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(40) << left << "SPEED DELIVERY - GESTIÓN" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 1. Clientes" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 2. Repartidores" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Sectores" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 4. Volver" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                clientesCRUD(sistema);
                break;
            } case 2: {
                repartidoresCRUD(sistema);
                break;
            } case 3: {
                sectoresCRUD(sistema);
                break;
            } case 4: {
                break;
            } default: {
                cout<< "Opcion inválida"<<endl;
            }
        }

    } while (opcion != 4);
}

void menuPrincipal(SistemaDelivery& sistema) {
    int opcion;

    // cargar datos de los archivos de texto
    sistema.cargarSectores();
    sistema.cargarClientes();
    sistema.cargarRepartidores();
    sistema.cargarDeliveries();

    // cargar datos
    if (sistema.sinDatos()) {
        sistema.cargarDatosEjemplo();
    }

    sistema.cargarGrafo();

    do {
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(40) << left << "SPEED DELIVERY - MENÚ PRINCIPAL" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 1. Servicio Diario" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(40) << left << " 2. Gestión" << right <<"|" << endl ;
        cout << "|" << setfill(' ') << setw(39) << left << " 3. Salir" << right <<"|" << endl ;
        cout << "|" << setfill('-') << setw(40) << right <<"|" << endl ;
        cout << "Elija la operación a realizar: ";
        opcion = leerOpcion();

        switch (opcion) {
            case 1: {
                menuServicioDiario(sistema);
                break;
            } case 2: {
                menuGestion(sistema);
                break;
            } case 3: {
                // guardar datos
                sistema.guardarSectores();
                sistema.guardarClientes();
                sistema.guardarRepartidores();
                sistema.guardarDeliveries();

                // generar txt de reporte
                sistema.generarReporte();
                cout << setfill('-') << setw(40) << "" << endl ;
                cout << "Gracias por usar el programa!" << endl;
                cout << setfill('-') << setw(40) << "" << endl ;
                break;
            } default: {
                cout<< "Opcion invalida"<<endl;
                break;
            }
        }

    } while (opcion != 3);
}



int main() {

    // sembrar semilla para generar números aleatorios
    srand(time(0));

    // Configurar consola a UTF-8 para que imprima acentos y caracteres especiales
    #ifdef _WIN32
        SetConsoleCP(CP_UTF8);
        SetConsoleOutputCP(CP_UTF8);
    #endif

    // Crear objeto de la clase SistemaDelivery que contiene la lógica relacionada a los envíos
    SistemaDelivery sistema;

    // Mostrar menú principal
    menuPrincipal(sistema);

    return 0;
};