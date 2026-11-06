leaf_info() {
    printf '[leaf] %s\n' "$*"
}

leaf_warn() {
    printf '[leaf] warning: %s\n' "$*" >&2
}

leaf_error() {
    printf '[leaf] error: %s\n' "$*" >&2
}

leaf_die() {
    leaf_error "$*"
    exit 1
}

leaf_has_command() {
    command -v "$1" >/dev/null 2>&1
}

leaf_append_unique() {
    case " ${LEAF_UNIQUE_LIST:-} " in
        *" $1 "*)
            ;;
        *)
            if [ -n "${LEAF_UNIQUE_LIST:-}" ]; then
                LEAF_UNIQUE_LIST="${LEAF_UNIQUE_LIST} $1"
            else
                LEAF_UNIQUE_LIST="$1"
            fi
            ;;
    esac
}

print_leaf_help() {
    cat <<'EOF'
Usage:
  leaf doctor [--target <target>...] [--strict]
  leaf device [--target <target>...]
  leaf clean  [--target <target>...] [--all]
  leaf build  [--target <target>...] [--mode debug|release] [--strict]
  leaf run    [--target <target>...] [--mode debug|release]
              [--device <id>] [--strict]
  leaf help
  leaf version

Targets:
  macos ios web android ohos

The default target is all. Values passed after --target may be separated by
spaces. The POSIX Shell CLI does not include the windows target.

Options:
  --target <target>...  Select one or more targets; defaults to all.
  --mode <mode>         Build mode for build and run; defaults to debug.
  --strict              Fail when any selected target is unsupported.
  --device <id>         Device identifier for run.
  --all                 In clean, also remove native build directories.
EOF
}

parse_leaf_arguments() {
    if [ "$#" -eq 0 ]; then
        LEAF_COMMAND=help
        LEAF_TARGET_REQUEST=all
        LEAF_TARGET_SPECIFIED=0
        LEAF_MODE=debug
        LEAF_STRICT=0
        LEAF_CLEAN_ALL=0
        LEAF_DEVICE_ID=
        return
    fi

    LEAF_COMMAND=$1
    shift

    case "${LEAF_COMMAND}" in
        -h|--help)
            LEAF_COMMAND=help
            LEAF_TARGET_REQUEST=all
            LEAF_TARGET_SPECIFIED=0
            LEAF_MODE=debug
            LEAF_STRICT=0
            LEAF_CLEAN_ALL=0
            LEAF_DEVICE_ID=
            return
            ;;
    esac

    LEAF_TARGET_REQUEST=
    LEAF_TARGET_SPECIFIED=0
    LEAF_TARGET_VALUE_COUNT=0
    LEAF_TARGET_OPTION_COUNT=0
    LEAF_READING_TARGETS=0
    LEAF_MODE=debug
    LEAF_STRICT=0
    LEAF_CLEAN_ALL=0
    LEAF_DEVICE_ID=

    while [ "$#" -gt 0 ]; do
        LEAF_ARG=$1
        shift

        if [ "${LEAF_READING_TARGETS}" -eq 1 ]; then
            case "${LEAF_ARG}" in
                -*)
                    if [ "${LEAF_TARGET_OPTION_COUNT}" -eq 0 ]; then
                        leaf_die "--target requires at least one target"
                    fi
                    LEAF_READING_TARGETS=0
                    ;;
                *)
                    if [ -n "${LEAF_TARGET_REQUEST}" ]; then
                        LEAF_TARGET_REQUEST="${LEAF_TARGET_REQUEST} ${LEAF_ARG}"
                    else
                        LEAF_TARGET_REQUEST="${LEAF_ARG}"
                    fi
                    LEAF_TARGET_SPECIFIED=1
                    LEAF_TARGET_VALUE_COUNT=$((LEAF_TARGET_VALUE_COUNT + 1))
                    LEAF_TARGET_OPTION_COUNT=$((LEAF_TARGET_OPTION_COUNT + 1))
                    continue
                    ;;
            esac
        fi

        case "${LEAF_ARG}" in
            --target)
                LEAF_READING_TARGETS=1
                LEAF_TARGET_OPTION_COUNT=0
                ;;
            --mode)
                case "${LEAF_COMMAND}" in
                    build|run)
                        ;;
                    *)
                        leaf_die "--mode is only valid for build and run"
                        ;;
                esac
                [ "$#" -gt 0 ] || leaf_die "--mode requires a value"
                LEAF_MODE=$1
                shift
                ;;
            --strict)
                case "${LEAF_COMMAND}" in
                    doctor|build|run)
                        LEAF_STRICT=1
                        ;;
                    *)
                        leaf_die "--strict is only valid for doctor, build, and run"
                        ;;
                esac
                ;;
            --device)
                if [ "${LEAF_COMMAND}" != "run" ]; then
                    leaf_die "--device is only valid for run"
                fi
                [ "$#" -gt 0 ] || leaf_die "--device requires a value"
                LEAF_DEVICE_ID=$1
                shift
                ;;
            --all)
                if [ "${LEAF_COMMAND}" != "clean" ]; then
                    leaf_die "--all is only valid for clean"
                fi
                LEAF_CLEAN_ALL=1
                ;;
            -h|--help)
                LEAF_COMMAND=help
                return
                ;;
            -*)
                leaf_die "unknown option '${LEAF_ARG}'"
                ;;
            *)
                leaf_die "unexpected argument '${LEAF_ARG}'"
                ;;
        esac
    done

    if [ "${LEAF_READING_TARGETS}" -eq 1 ] &&
       [ "${LEAF_TARGET_OPTION_COUNT}" -eq 0 ]; then
        leaf_die "--target requires at least one target"
    fi

    if [ "${LEAF_TARGET_SPECIFIED}" -eq 0 ]; then
        LEAF_TARGET_REQUEST=all
    fi

    case "${LEAF_COMMAND}" in
        doctor|device|clean|build|run|help|version)
            ;;
        *)
            leaf_die "unknown command '${LEAF_COMMAND}'"
            ;;
    esac

    case "${LEAF_MODE}" in
        debug|release)
            ;;
        *)
            leaf_die "unsupported mode '${LEAF_MODE}'; expected debug or release"
            ;;
    esac
}
