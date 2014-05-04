#!/usr/bin/env python3
# filter.py
#
# CTF Event pretty printer
#
# Copyright 2014 Jérémie Galarneau <jeremie.galarneau@gmail.com>
#
# Author: Jérémie Galarneau <jeremie.galarneau@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

from babeltrace import Event
from babeltrace import CTFScope

def print_event(event):
		output_string = ""

		output_string += "[{}] {}: ".format(event.timestamp, event.name)

		# Check if cpu_id is available in the STREAM_PACKET_CONTEXT...
		try:
				cpu_id = event["cpu_id"]
		except KeyError:
				cpu_id = None

		if cpu_id is not None:
			output_string += "{{{} = {}}}, ".format("cpu_id", cpu_id)

		for field in event.field_list_with_scope(CTFScope.EVENT_FIELDS):
				output_string += "{{{} = {}}}, ".format(field, event[field])

		print(output_string[:-2])
