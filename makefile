!include <win32.mak>

srcs_dir=srcs
objs_dir=objs
keylogger=winkey.exe

cflags=$(cflags) /EHsc -DUNICODE -D_UNICODE

all: $(keylogger)

clean:
	@if exist $(objs_dir) rmdir /s /q $(objs_dir)

fclean: clean
	@if exist $(keylogger) del $(keylogger)

$(keylogger): $(objs_dir)\winkey.obj
	$(link) $(ldebug) $(lflags) $(conflags) $** -out:$@ kernel32.lib user32.lib

{$(srcs_dir)\}.cpp{$(objs_dir)\}.obj:
	@if not exist $(objs_dir) mkdir $(objs_dir)

	$(cc) $(cdebug) $(cflags) $(cvars) /Fo"$(objs_dir)\\" /c $<