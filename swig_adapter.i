%module swig_adapter

%{
#include <babeltrace/ctf/events.h>

struct bt_ctf_event *cast_to_bt_ctf_event(size_t ptr)
{
	return (struct bt_ctf_event *) ptr;
}
%}

%rename("_cast_to_bt_ctf_event") cast_to_bt_ctf_event(size_t ptr);
struct bt_ctf_event *cast_to_bt_ctf_event(size_t ptr);
