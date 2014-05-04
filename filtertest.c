/*
 * filtertest.c
 *
 * Python-based CTF event filtering demo
 *
 * Copyright 2014 Jérémie Galarneau <jeremie.galarneau@gmail.com>
 *
 * Author: Jérémie Galarneau <jeremie.galarneau@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define FILTER_PY "filter.py"
#define FORMATTER_PY "formatter.py"
#define SWIG_ADAPTER_PY "swig_adapter.py"

#define PY_FILTER_FUNC "on_event"
#define PY_FORMATTER_FUNC "print_event"
#define PY_SWIG_ADAPTER_FUNC "_cast_to_bt_ctf_event"

#include <babeltrace/babeltrace.h>
#include <babeltrace/ctf/iterator.h>
#include <Python.h>
#include <stdio.h>

struct python_ctx {
	PyObject *main_mod;
	PyObject *main_dict;
	PyObject *filter_func;
	PyObject *formatter_func;
	PyObject *swig_adapter_func;
	/* Reuse the same event instance, only changing the native pointer */
	PyObject *event;
};

void python_ctx_destroy(struct python_ctx *ctx)
{
	if (!ctx) {
		goto end;
	}

	Py_XDECREF(ctx->event);
	free(ctx);
end:
	Py_Finalize();
}

struct python_ctx *python_ctx_create()
{
	struct python_ctx *ctx;
	int ret;
	FILE *fp;

	Py_Initialize();

	ctx = malloc(sizeof(struct python_ctx));
	if (!ctx) {
		perror("malloc");
		goto end;
	}

	memset(ctx, 0, sizeof(struct python_ctx));
	ctx->main_mod = PyImport_AddModule("__main__");
	if (!ctx->main_mod) {
		fprintf(stderr, "[Error] Failed to import __main__ module\n");
		goto error;
	}

	ctx->main_dict = PyModule_GetDict(ctx->main_mod);
	if (!ctx->main_dict) {
		fprintf(stderr, "[Error] Failed to get main dictionary\n");
		goto error;
	}

	fp = fopen(FILTER_PY, "r");
	if (!fp) {
		perror("fopen");
		goto error;
	}

	ret = PyRun_SimpleFileEx(fp, FILTER_PY, 1);
	if (ret) {
		fprintf(stderr, "[Error] Failed to parse filter\n");
		goto error;
	}

	fp = fopen(FORMATTER_PY, "r");
	if (!fp) {
		perror("fopen");
		goto error;
	}

	ret = PyRun_SimpleFileEx(fp, FORMATTER_PY, 1);
	if (ret) {
		fprintf(stderr, "[Error] Failed to parse formatter\n");
		goto error;
	}

	fp = fopen(SWIG_ADAPTER_PY, "r");
	if (!fp) {
		perror("fopen");
		goto error;
	}

	ret = PyRun_SimpleFileEx(fp, SWIG_ADAPTER_PY, 1);
	if (ret) {
		fprintf(stderr, "[Error] Failed to parse swig adapter\n");
		goto error;
	}

	ctx->formatter_func = PyDict_GetItemString(ctx->main_dict,
		PY_FORMATTER_FUNC);
	if (!ctx->formatter_func) {
		fprintf(stderr, "[Error] Could not find %s function\n",
			PY_FORMATTER_FUNC);
		goto error;
	}

	ctx->filter_func = PyDict_GetItemString(ctx->main_dict,
		PY_FILTER_FUNC);
	if (!ctx->filter_func) {
		fprintf(stderr, "[Error] Could not find %s function\n",
			PY_FILTER_FUNC);
		goto error;
	}

	ctx->swig_adapter_func = PyDict_GetItemString(ctx->main_dict,
		PY_SWIG_ADAPTER_FUNC);
	if (!ctx->swig_adapter_func) {
		fprintf(stderr, "[Error] Could not find %s function\n",
			PY_SWIG_ADAPTER_FUNC);
		goto error;
	}

	/*
	 * There are, assuredly, better ways of doing this....
	 * pull request/patch welcome...
	 */
	ctx->event = PyRun_String("Event.__new__(Event)", Py_eval_input,
		ctx->main_dict, ctx->main_dict);
	if (!ctx->event) {
		fprintf(stderr, "Could not instantiate Event class\n");
		goto error;
	}

end:
	return ctx;
error:
	python_ctx_destroy(ctx);
	return NULL;
}

int filter_event(struct python_ctx *ctx)
{
	int ret = 0;
	PyObject *py_ret;

	if (!ctx || !ctx->event || !ctx->filter_func) {
		ret = -1;
		goto end;
	}

	py_ret = PyObject_CallFunctionObjArgs(ctx->filter_func, ctx->event, NULL);
	if (!py_ret) {
		ret = -1;
		fprintf(stderr,
			"[Error] Python filter function invocation failed\n");
		PyErr_PrintEx(0);
		goto end;
	}

	if (!PyBool_Check(py_ret)) {
		fprintf(stderr,
			"[Error] Invalid type returned from Python filter function\n");
	}

	ret = !!(py_ret == Py_True);
	Py_DECREF(py_ret);
end:
	return ret;
}

int print_event(struct python_ctx *ctx)
{
	int ret = 0;
	PyObject *py_ret;

	if (!ctx || !ctx->event || !ctx->formatter_func) {
		ret = -1;
		goto end;
	}

	py_ret = PyObject_CallFunctionObjArgs(ctx->formatter_func, ctx->event, NULL);
	if (!py_ret) {
		ret = -1;
		fprintf(stderr,
			"[Error] Python formatter function invocation failed\n");
		PyErr_PrintEx(0);
		goto end;
	}

	Py_DECREF(py_ret);
end:
	return ret;
}

int main(int argc, char **argv)
{
	int ret = 0;
	struct bt_context *bt_ctx;
	struct python_ctx *py_ctx = NULL;
	struct bt_ctf_iter *it = NULL;
	struct bt_ctf_event *event = NULL;
	struct bt_iter_pos pos = { .type = BT_SEEK_BEGIN, .u.seek_time = 0 };

	if (argc < 2) {
		printf("Usage: filtertest /path/to/trace\n");
	}

	bt_ctx = bt_context_create();
	if (!bt_ctx) {
		ret = -1;
		fprintf(stderr, "[Error] Context creation failed\n");
		goto end;
	}

	ret = bt_context_add_trace(bt_ctx, argv[1], "ctf", NULL, NULL, NULL);
	if (ret < 0) {
		fprintf(stderr, "[Error] Failed to open trace at %s\n",
			argv[1]);
		goto end;
	}

	/*
	 * Iterate over all events, handing them to python for filtering and
	 * pretty printing.
	 */
	it = bt_ctf_iter_create(bt_ctx, &pos, NULL);
	if (!it) {
		ret = -1;
		fprintf(stderr, "[Error] Iterator creation failed\n");
	}

	py_ctx = python_ctx_create();
	if (!py_ctx) {
		ret = -1;
		goto end;
	}

	while ((event = bt_ctf_iter_read_event(it))) {
		PyObject *event_swig_ptr;
		PyObject *event_size_t = PyLong_FromSize_t((size_t) event);

		/*
		 * We can't simply pass the event pointer to the Event class; it
		 * must be wrapped as a SWIG raw pointer object which contains
		 * type information. The swig_adapter library serves the purpose
		 * of converting our raw pointer to a SWIG object used
		 * internally by the Babeltrace bindings.
		 */
		if (!event_size_t) {
			ret = -1;
			fprintf(stderr,
				"[Error] Could not wrap native pointer as size_t\n");
			goto end;
		}

		event_swig_ptr = PyObject_CallFunctionObjArgs(
			py_ctx->swig_adapter_func, event_size_t, NULL);
		Py_DECREF(event_size_t);
		if (!event_swig_ptr) {
			fprintf(stderr, "[Error] Call to %s failed",
				PY_SWIG_ADAPTER_FUNC);
		}
		ret = PyObject_SetAttrString(py_ctx->event, "_e",
			event_swig_ptr);

		Py_XDECREF(event_swig_ptr);
		if (ret < 0) {
			fprintf(stderr, "[Error] Could not set _e attribute\n");
			goto end;
		}

		ret = filter_event(py_ctx);
		if (ret < 0) {
			fprintf(stderr, "[Error] Filter evaluation failed\n");
			goto end;
		}

		if (ret) {
			print_event(py_ctx);
		}

		ret = bt_iter_next(bt_ctf_get_iter(it));
		if (ret < 0) {
			goto end;
		}
	}

	ret = 0;
end:
	if (it) {
		bt_ctf_iter_destroy(it);
	}

	if (bt_ctx) {
		bt_context_put(bt_ctx);
	}

	python_ctx_destroy(py_ctx);
	return ret;
}
