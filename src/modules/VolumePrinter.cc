#include <err.h>
#include <iostream>
#include <array>

#include "../process_configuration.h"
#include "../alsa.h"

#include "VolumePrinter.hpp"

using namespace std::literals;

namespace swaystatus::modules {
class VolumePrinter: public Base {
    ;

public:
    VolumePrinter(void *config, const char *mix_name, const char *card):
        Base{
            config, "VolumePrinter"sv,
            1, "vol: {muted} {volume}%", nullptr,
            "mix_name", "card"
        }
    {
        initialize_alsa_lib(mix_name, card);
    }

    void update()
    {
        update_volume();
    }
    void do_print(const char *format)
    {
        print(format, 
            fmt::arg("volume", get_audio_volume()),
            fmt::arg("muted", is_audio_muted())
            );
    }
    std::string execute_command(const char* cmd) {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        return result;
    }
    std::string is_audio_muted() {
        try {
            std::string cmd_output = execute_command("pactl get-sink-mute @DEFAULT_SINK@");
            if (cmd_output.find("Mute: yes") == std::string::npos) {
                return "unmuted";
            } else {
                return "muted";
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Error checking mute status: " << e.what() << '\n';
            return "unknown";
        }
    }
    void reload()
    {}
};

std::unique_ptr<Base> makeVolumePrinter(void *config)
{
    std::unique_ptr<const char[]> mix_name{get_property(config, "mix_name", "Master")};
    std::unique_ptr<const char[]> card    {get_property(config, "card",     "default")};

    return std::make_unique<VolumePrinter>(config, mix_name.get(), card.get());
}
} /* namespace swaystatus::modules */
