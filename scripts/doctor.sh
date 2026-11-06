run_doctor() {
    resolve_leaf_targets "${LEAF_TARGET_REQUEST}" || exit 1

    printf '%-10s %-12s %s\n' "TARGET" "STATUS" "DETAIL"
    LEAF_DOCTOR_HAS_UNSUPPORTED=0

    for LEAF_DOCTOR_TARGET in ${LEAF_RESOLVED_TARGETS}; do
        if leaf_target_doctor "${LEAF_DOCTOR_TARGET}"; then
            printf '%-10s %-12s %s\n' \
                    "${LEAF_DOCTOR_TARGET}" "${TARGET_STATUS}" "${TARGET_REASON}"
        else
            printf '%-10s %-12s %s\n' \
                    "${LEAF_DOCTOR_TARGET}" "${TARGET_STATUS}" "${TARGET_REASON}"
            LEAF_DOCTOR_HAS_UNSUPPORTED=1
        fi
    done

    if [ "${LEAF_DOCTOR_HAS_UNSUPPORTED}" -eq 1 ]; then
        if [ "${LEAF_STRICT}" -eq 1 ] ||
           [ "${LEAF_TARGET_REQUEST}" != "all" ]; then
            return 1
        fi
    fi

    return 0
}
