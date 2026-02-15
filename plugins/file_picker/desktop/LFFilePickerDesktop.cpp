#include "../cpp/LFFilePicker.h"

#include <cctype>
#include <cstdio>

namespace {

std::string trim(const std::string& input) {
    size_t start = 0;
    while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start]))) {
        ++start;
    }

    size_t end = input.size();
    while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }
    return input.substr(start, end - start);
}

std::string runCommand(const std::string& command) {
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    char buffer[512];
    std::string output;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        output += buffer;
    }

    pclose(pipe);
    return trim(output);
}

}

bool lfFilePickerRequestFromPlatform(int requestId, std::string& error) {
    (void) error;

    const std::string path = runCommand(
            "osascript -e 'try' "
            "-e 'POSIX path of (choose file with prompt \"Select a file\")' "
            "-e 'on error number -128' "
            "-e 'return \"\"' "
            "-e 'end try'");

    if (path.empty()) {
        lfFilePickerOnPlatformResult(requestId, 0, "", "canceled");
    } else {
        lfFilePickerOnPlatformResult(requestId, 1, path.c_str(), "");
    }
    return true;
}
