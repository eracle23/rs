macro(TEST_QNANHIBIT VARIABLE LOCAL_TEST_DIR)
  # Modern platforms converge on IEEE-754 quiet NaN with the high mantissa bit set (bit 22 for 32-bit float).
  # Upstream Teem removed the runtime probe (try_run) and hard-codes this.
  # We align with upstream to avoid fragile configure-time execution.
  set(${VARIABLE} 1 CACHE STRING "Assume IEEE754 QNaNHiBit==1 on this platform" FORCE)
  set(HAVE_${VARIABLE} 1 CACHE INTERNAL "probe skipped" FORCE)
  message(STATUS "Assume QNaNHiBit==1 (skip runtime probe)")
endmacro()

