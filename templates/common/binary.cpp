
template <typename T>
T read(std::ifstream &input) {
    T value;
    input.read(reinterpret_cast<char*>(&value), sizeof(T));
    if (!input) {
        throw std::runtime_error("Error reading from file");
    }
    return value;
}
template <typename T>
void write(std::ofstream &output, const T &value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(T));
    if (!output) {
        throw std::runtime_error("Error writing to file");
    }
}