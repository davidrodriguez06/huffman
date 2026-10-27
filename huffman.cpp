#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>

using namespace std;

struct MinNode {
    char caracter;
    unsigned frecuencia;
    MinNode *izquierda, *derecha;

    MinNode(char caracter, unsigned frecuencia) {
        izquierda = derecha = nullptr;
        this->caracter = caracter;
        this->frecuencia = frecuencia;
    }

    ~MinNode() {
        delete izquierda;
        delete derecha;
    }
};

struct Comparar {
    bool operator()(MinNode* l, MinNode* r) {
        return (l->frecuencia > r->frecuencia);
    }
};

void generarCodigos(MinNode* raiz, string str,
                    unordered_map<char, string>& codigosHuffman) {
    if (!raiz) return;
    if (raiz->caracter != '\0')
        codigosHuffman[raiz->caracter] = str;

    generarCodigos(raiz->izquierda, str + "0", codigosHuffman);
    generarCodigos(raiz->derecha,   str + "1", codigosHuffman);
}
//Formato preorden binario:
//'I' = nodo interno (siguen sus hijos)
//'H' = hoja        (siguen 1 byte con el carácter)
//'N' = nullptr     (solo en el caso de un único carácter)
void serializarArbol(MinNode* raiz, ofstream& archivo) {
    if (!raiz) {
        char marca = 'N';
        archivo.write(&marca, 1);
        return;
    }
    if (raiz->caracter != '\0') {
        char marca = 'H';
        archivo.write(&marca, 1);
        archivo.write(&raiz->caracter, 1);
    } else {
        char marca = 'I';
        archivo.write(&marca, 1);
        serializarArbol(raiz->izquierda, archivo);
        serializarArbol(raiz->derecha, archivo);
    }
}

MinNode* deserializarArbol(ifstream& archivo) {
    char marca;
    if (!archivo.read(&marca, 1)) return nullptr;

    if (marca == 'N') return nullptr;

    if (marca == 'H') {
        char c;
        archivo.read(&c, 1);
        return new MinNode(c, 0);
    }
    // 'I'
    MinNode* nodo = new MinNode('\0', 0);
    nodo->izquierda = deserializarArbol(archivo);
    nodo->derecha   = deserializarArbol(archivo);
    return nodo;
}

void codificarHuffman(const string& texto, const string& nombreArchivo) {
    if (texto.empty()) {
        cout << "Texto vacío, nada que codificar." << endl;
        return;
    }

    // 1. Frecuencias
    unordered_map<char, int> frecuenciaMapa;
    for (char c : texto) frecuenciaMapa[c]++;

    // 2. Min-Heap
    priority_queue<MinNode*, vector<MinNode*>, Comparar> minHeap;
    for (auto par : frecuenciaMapa)
        minHeap.push(new MinNode(par.first, par.second));

    // Caso especial: un único carácter
    if (minHeap.size() == 1) {
        MinNode* izq = minHeap.top(); minHeap.pop();
        MinNode* sumNode = new MinNode('\0', izq->frecuencia);
        sumNode->izquierda = izq;
        minHeap.push(sumNode);
    }

    // 3. Construir árbol
    while (minHeap.size() > 1) {
        MinNode* izq = minHeap.top(); minHeap.pop();
        MinNode* der = minHeap.top(); minHeap.pop();
        MinNode* sumNode = new MinNode('\0', izq->frecuencia + der->frecuencia);
        sumNode->izquierda = izq;
        sumNode->derecha   = der;
        minHeap.push(sumNode);
    }
    MinNode* raiz = minHeap.top();

    // 4. Tabla de códigos
    unordered_map<char, string> codigosHuffman;
    generarCodigos(raiz, "", codigosHuffman);

    cout << "=== TABLA DE CODIGOS DE HUFFMAN ===" << endl;
    for (auto& par : codigosHuffman)
        cout << par.first << " : " << par.second << endl;

    // 5. Texto codificado en bits (string de 0s y 1s)
    string strCodificado;
    for (char c : texto) strCodificado += codigosHuffman[c];

    cout << "\nTexto original: " << texto << endl;
    cout << "Texto codificado: " << strCodificado << endl;

    // 6. GUARDAR EN FICHERO BINARIO
    ofstream archivo(nombreArchivo, ios::binary);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo para escritura." << endl;
        delete raiz;
        return;
    }

    // 6.1 Longitud original (unsigned int, 4 bytes)
    unsigned int longitudOriginal = static_cast<unsigned int>(texto.size());
    archivo.write(reinterpret_cast<const char*>(&longitudOriginal),
                  sizeof(longitudOriginal));

    // 6.2 Árbol de Huffman serializado
    serializarArbol(raiz, archivo);

    // 6.3 Bits empaquetados en bytes (padding con ceros al final)
    unsigned char byte = 0;
    int bitsEnByte = 0;
    for (char bit : strCodificado) {
        byte = (byte << 1) | (bit == '1' ? 1 : 0);
        if (++bitsEnByte == 8) {
            archivo.write(reinterpret_cast<const char*>(&byte), 1);
            byte = 0;
            bitsEnByte = 0;
        }
    }
    if (bitsEnByte > 0) {
        byte <<= (8 - bitsEnByte);
        archivo.write(reinterpret_cast<const char*>(&byte), 1);
    }

    archivo.close();
    cout << "\nFichero comprimido guardado en: " << nombreArchivo << endl;

    delete raiz;
}

string descomprimir(const string& nombreArchivo) {
    ifstream archivo(nombreArchivo, ios::binary);
    if (!archivo.is_open()) {
        cerr << "No se pudo abrir el archivo para lectura." << endl;
        return "";
    }

    // 1. Longitud original
    unsigned int longitudOriginal;
    if (!archivo.read(reinterpret_cast<char*>(&longitudOriginal),
                      sizeof(longitudOriginal))) {
        cerr << "Error leyendo la longitud." << endl;
        return "";
    }

    // 2. Reconstruir el árbol
    MinNode* raiz = deserializarArbol(archivo);
    if (!raiz) {
        cerr << "Error leyendo el árbol." << endl;
        return "";
    }

    // 3. Decodificar bits
    string resultado;
    MinNode* actual = raiz;
    unsigned char byte;

    while (resultado.size() < longitudOriginal &&
           archivo.read(reinterpret_cast<char*>(&byte), 1)) {
        for (int i = 7; i >= 0 && resultado.size() < longitudOriginal; i--) {
            int bit = (byte >> i) & 1;
            actual = (bit == 0) ? actual->izquierda : actual->derecha;

            if (!actual) { // árbol corrupto
                delete raiz;
                return "";
            }
            if (actual->caracter != '\0') {
                resultado += actual->caracter;
                actual = raiz;
            }
        }
    }

    archivo.close();
    delete raiz;
    return resultado;
}

int main() {
    string texto = "ejemplo de codificacion huffman en c++";
    string nombreArchivo = "comprimido.huff";

    codificarHuffman(texto, nombreArchivo);

    string recuperado = descomprimir(nombreArchivo);
    cout << "\n=== TEXTO DESCOMPRIMIDO ===" << endl;
    cout << recuperado << endl;

    cout << "\nVerificación: "
         << (recuperado == texto ? "OK ✓" : "FALLO ✗") << endl;

    return 0;
}