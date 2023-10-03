#include <cstdio>
#include <cstring>
#include <unordered_map>
#include <variant>
#include <vector>
#include <functional>
#include <regex>
#include <stdexcept>
#include <memory_resource>
#include <fmt/format.h>
//#include <fmt/ostream.h>
//#include <pmr/string>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Define a polymorphic resource-aware string type.
using PmrString = std::pmr::string;

// Define the types for cvar values using std::variant.
using CVarValue = std::variant<int, float, PmrString, std::pmr::vector<int>, std::pmr::vector<float>>;

// Define a struct to hold cvar information, including default values and constraints.
struct CVarInfo {
    CVarValue value;
    CVarValue defaultValue;
    CVarValue minValue;
    CVarValue maxValue;
    PmrString description;
    std::regex validationRegex;
    std::function<void(const CVarValue&)> changeCallback;
};

/*
// Define a helper function to memory-map a file on Windows.
bool MemoryMapFile(const char* filename, const void*& mappedData, size_t& fileSize) {
#ifdef _WIN32
    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        fmt::print(stderr, "Error: Unable to open memory-mapped file {}\n", filename);
        return false;
    }

    HANDLE fileMapping = CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!fileMapping) {
        CloseHandle(fileHandle);
        fmt::print(stderr, "Error: Unable to create file mapping for {}\n", filename);
        return false;
    }

    mappedData = MapViewOfFile(fileMapping, FILE_MAP_READ, 0, 0, 0);
    if (!mappedData) {
        CloseHandle(fileMapping);
        CloseHandle(fileHandle);
        fmt::print(stderr, "Error: Unable to map view of file {}\n", filename);
        return false;
    }

    LARGE_INTEGER fileSizeInfo;
    if (!GetFileSizeEx(fileHandle, &fileSizeInfo)) {
        UnmapViewOfFile(mappedData);
        CloseHandle(fileMapping);
        CloseHandle(fileHandle);
        fmt::print(stderr, "Error: Unable to get file size for {}\n", filename);
        return false;
    }
    fileSize = static_cast<size_t>(fileSizeInfo.QuadPart);
#else
    int fileDescriptor = open(filename, O_RDONLY);
    if (fileDescriptor == -1) {
        fmt::print(stderr, "Error: Unable to open memory-mapped file {}\n", filename);
        return false;
    }

    // Get the file size.
    off_t fileSizeLong = lseek(fileDescriptor, 0, SEEK_END);
    lseek(fileDescriptor, 0, SEEK_SET);
    fileSize = static_cast<size_t>(fileSizeLong);

    // Map the file into memory.
    mappedData = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fileDescriptor, 0);
    close(fileDescriptor);

    if (mappedData == MAP_FAILED) {
        fmt::print(stderr, "Error: Unable to map memory for {}\n", filename);
        return false;
    }
#endif
    return true;
}

// Define a helper function to unmap a memory-mapped file.
void UnmapFile(const void* mappedData, size_t fileSize) {
#ifdef _WIN32
    UnmapViewOfFile(mappedData);
#else
    munmap(const_cast<void*>(mappedData), fileSize);
#endif
}*/

// Define the cvar system.
/*class CVarSystem {
public:
    // Define a map to store cvars.
    std::unordered_map<PmrString, CVarInfo> cvars;

    // Constructor to initialize cvar system.
    explicit CVarSystem(std::pmr::memory_resource* resource)
        : resource_(resource) {
        // Register "cvar_buffer_size" with a default value.
        RegisterCVar<int>("cvar_buffer_size", defaultBufferSize, defaultBufferSize, 1, 1024,
                          "CVar buffer size", R"(\d+)");
    }

      // Register a cvar with a value and optional default, min, and max values.
    template <typename T>
    void RegisterCVar(const PmrString& name, const CVarValue& value, const CVarValue& defaultValue = T{},
                      const CVarValue& minValue = std::numeric_limits<T>::min(),
                      const CVarValue& maxValue = std::numeric_limits<T>::max(),
                      const PmrString& description = "",
                      const std::string& validationRegex = ".*",
                      std::function<void(const CVarValue&)> changeCallback = nullptr) {
        cvars[name, resource_] = {defaultValue, value, minValue, maxValue, description, std::regex(validationRegex),
                                  changeCallback};
    }

    // Get the value of a cvar.
    template <typename T>
    T GetCVar(const PmrString& name) const {
        const auto& cvarInfo = cvars.at(name);
        return std::get<T>(cvarInfo.value);
    }

    // Set the value of a cvar.
    template <typename T>
    void SetCVar(const PmrString& name, const T& newValue) {
        auto& cvarInfo = cvars.at(name);
        if (std::regex_match(std::to_string(newValue), cvarInfo.validationRegex) &&
            newValue >= std::get<T>(cvarInfo.minValue) && newValue <= std::get<T>(cvarInfo.maxValue)) {
            cvarInfo.value = newValue;
            if (cvarInfo.changeCallback) {
                cvarInfo.changeCallback(cvarInfo.value);
            }
        } else {
            throw std::runtime_error(fmt::format("Invalid value for cvar: {}", name));
        }
    }

    // Load cvars from a memory-mapped file.
    bool LoadCVarConfigFromMemoryMap(const char* filename) {
        const void* mappedData = nullptr;
        size_t fileSize = 0;

        if (!MemoryMapFile(filename, mappedData, fileSize)) {
            return false;
        }

        // Now you can interpret the mappedData as your data structure and populate cvars.

        // Unmap the memory-mapped file.
        UnmapFile(mappedData, fileSize);
        return true;
    }

private:
    std::pmr::memory_resource* resource_;
    const int defaultBufferSize = 1024;  // Default buffer size.
};

int main() {
    // Create a memory resource for the CVarSystem.
    std::array<char, 1024> buffer;  // Statically allocated memory pool.
    std::pmr::monotonic_buffer_resource resource(buffer.data(), buffer.size());
    CVarSystem cvarSystem(&resource);

    // Load cvars from a memory-mapped file, which includes "cvar_buffer_size."
    if (cvarSystem.LoadCVarConfigFromMemoryMap("config.bin")) {
        // Get the buffer size from the loaded cvar or use the default if not specified.
        int bufferSize = cvarSystem.GetCVar<int>("cvar_buffer_size");

        // Create a buffer of the specified size or use the default.
        std::vector<char> dynamicBuffer(bufferSize, ' ');

        // Register cvars with pmr-aware description.
        cvarSystem.RegisterCVar<int>("sv_gravity", 800, 800, 100, 2000,
                                      "Gravity strength", R"(\d+)");
        cvarSystem.RegisterCVar<float>("cl_fov", 90.0f, 90.0f, 60.0f, 120.0f,
                                       "Field of view", R"(\d+(\.\d+)?)");
        cvarSystem.RegisterCVar<PmrString>("player_name", "Player1",
                                           "Default player name", R"(\w+)");
        cvarSystem.RegisterCVar<std::pmr::vector<int>>("player_scores",
                                                       std::pmr::vector<int>(&resource, {100, 200, 300}), {},
                                                       {}, "Player scores", R"(\d+)");
        cvarSystem.RegisterCVar<std::pmr::vector<float>>("player_positions",
                                                         std::pmr::vector<float>(&resource, {0.0f, 0.0f, 0.0f}),
                                                         {}, {}, "Player positions", R"(\d+(\.\d+)?)");

        // Set a callback for cl_fov changes.
        cvarSystem.cvars["cl_fov"].changeCallback = [](const CVarValue& newValue) {
            fmt::print("cl_fov changed to: {}\n", std::get<float>(newValue));
        };

        // Query and print cvar values.
        fmt::print("sv_gravity: {}\n", cvarSystem.GetCVar<int>("sv_gravity"));
        fmt::print("cl_fov: {}\n", cvarSystem.GetCVar<float>("cl_fov"));
        fmt::print("player_name: {}\n", cvarSystem.GetCVar<PmrString>("player_name"));
        fmt::print("player_scores: ");
        for (const auto& score : cvarSystem.GetCVar<std::pmr::vector<int>>("player_scores")) {
            fmt::print("{} ", score);
        }
        fmt::print("\n");

        // Set and print a cvar value.
        try {
            cvarSystem.SetCVar<float>("cl_fov", 70.0f);
            fmt::print("Updated cl_fov: {}\n", cvarSystem.GetCVar<float>("cl_fov"));
        } catch (const std::exception& e) {
            fmt::print(stderr, "Error: {}\n", e.what());
        }
    }

    return 0;
}*/


int main(int argc, char *argv[]) {
    //std::cout << "argc == " << argc << '\n';
 
    //for (int ndx{}; ndx != argc; ++ndx)
    //    std::cout << "argv[" << ndx << "] == " << std::quoted(argv[ndx]) << '\n';
    //std::cout << "argv[" << argc << "] == " << static_cast<void*>(argv[argc]) << '\n';
  
    return EXIT_SUCCESS;// : EXIT_FAILURE; // optional return value
}