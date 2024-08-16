#include <gst/gst.h>
#include <gst/gstelement.h>

/**
 * gst_element_get_current_clock_time:
 * @element: a #GstElement.
 *
 * Returns the current clock time of the element, as in, the time of the
 * element's clock, or GST_CLOCK_TIME_NONE if there is no clock.
 *
 * Returns: the clock time of the element, or GST_CLOCK_TIME_NONE if there is
 * no clock.
 *
 * Since: 1.18
 */
GstClockTime
gst_element_get_current_clock_time (GstElement * element)
{
  GstClock *clock = NULL;
  GstClockTime ret;

  g_return_val_if_fail (GST_IS_ELEMENT (element), GST_CLOCK_TIME_NONE);

  clock = gst_element_get_clock (element);

  if (!clock) {
    GST_DEBUG_OBJECT (element, "Element has no clock");
    return GST_CLOCK_TIME_NONE;
  }

  ret = gst_clock_get_time (clock);
  gst_object_unref (clock);

  return ret;
}

/**
 * gst_element_get_current_running_time:
 * @element: a #GstElement.
 *
 * Returns the running time of the element. The running time is the
 * element's clock time minus its base time. Will return GST_CLOCK_TIME_NONE
 * if the element has no clock, or if its base time has not been set.
 *
 * Returns: the running time of the element, or GST_CLOCK_TIME_NONE if the
 * element has no clock or its base time has not been set.
 *
 * Since: 1.18
 */
GstClockTime
gst_element_get_current_running_time (GstElement * element)
{
  GstClockTime base_time, clock_time;

  g_return_val_if_fail (GST_IS_ELEMENT (element), GST_CLOCK_TIME_NONE);

  base_time = gst_element_get_base_time (element);

  if (!GST_CLOCK_TIME_IS_VALID (base_time)) {
    GST_DEBUG_OBJECT (element, "Could not determine base time");
    return GST_CLOCK_TIME_NONE;
  }

  clock_time = gst_element_get_current_clock_time (element);

  if (!GST_CLOCK_TIME_IS_VALID (clock_time)) {
    return GST_CLOCK_TIME_NONE;
  }

  if (clock_time < base_time) {
    GST_DEBUG_OBJECT (element, "Got negative current running time");
    return GST_CLOCK_TIME_NONE;
  }

  return clock_time - base_time;
}
