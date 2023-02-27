#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace cli {
    enum switch_t {
        SW_VERSION,
        SW_DISASSEMBLE,
        SW_HELP,
        SW_STDIN,
        SW_TRACE
    };

    enum setting_t {
        ST_INPUT,
        ST_MNEMONIC_SIZE,
        ST_OPCODE_SIZE,
        ST_LINE_SIZE,
        ST_MEMORY_SIZE,
        ST_MEMORY_BASE,
        ST_BIOS,
        ST_CPU_SPEED
    };

    class parser_t {
        int m_argc = 0;

        const char** m_argv = nullptr;

        std::unordered_map <setting_t, std::string> m_settings;
        std::unordered_map <switch_t , bool>        m_switches;

#define WSHORTHAND(shortname, longname, st) \
    { shortname, st }, \
    { longname , st }

#define LONG_ONLY(longname, st) { longname, st }

        std::unordered_map <std::string, switch_t> m_switches_map = {
            WSHORTHAND("-v", "--version"             , SW_VERSION            ),
            WSHORTHAND("-H", "--help"                , SW_HELP               ),
            WSHORTHAND("-d", "--disassemble"         , SW_DISASSEMBLE        ),
            WSHORTHAND("-t", "--trace"               , SW_TRACE              ),
            LONG_ONLY (      "--stdin"               , SW_STDIN              )
        };

        std::unordered_map <std::string, setting_t> m_settings_map = {
            WSHORTHAND("-i" , "--input"               , ST_INPUT              ),
            WSHORTHAND("-Sm", "--mnemonic-size"       , ST_MNEMONIC_SIZE      ),
            WSHORTHAND("-So", "--opcode-size"         , ST_OPCODE_SIZE        ),
            WSHORTHAND("-Sl", "--line-size"           , ST_LINE_SIZE          ),
            WSHORTHAND("-M" , "--memory"              , ST_MEMORY_SIZE        ),
            WSHORTHAND("-b" , "--bios"                , ST_BIOS               ),
            WSHORTHAND("-s" , "--cpu-speed"           , ST_CPU_SPEED          ),
            LONG_ONLY (       "--memory-base"         , ST_MEMORY_BASE        )
        };

#undef WSHORTHAND
#undef LONG_ONLY

    public:
        void init(int argc, const char* argv[]) {
            m_argc = argc;
            m_argv = argv;
        }

        bool get_switch(switch_t sw) {
            return m_switches.contains(sw);
        }

        bool is_set(setting_t st) {
            return m_settings.contains(st);
        }

        std::string get_setting(setting_t st) {
            return m_settings[st];
        }

        bool parse() {
            if (m_argc == 1) {
                return false;
            }

            for (int i = 1; i < m_argc; i++) {
                std::string arg(m_argv[i]);

                if (m_switches_map.contains(arg)) {
                    m_switches[m_switches_map[arg]] = true;

                    continue;
                }

                if (m_settings_map.contains(arg)) {
                    m_settings[m_settings_map[arg]] = std::string(m_argv[++i]);

                    continue;
                }

                if (m_settings.contains(ST_INPUT)) {
                    //ERROR(fmt("Unknown setting \"%s\"", arg.c_str()));
                } else {
                    m_settings[ST_INPUT] = arg;
                }
            }

            return true;
        }
    };
}