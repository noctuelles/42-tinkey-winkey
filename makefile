!include <win32.mak>

srcs_dir=srcs
objs_dir=objs
keylogger=winkey.exe
hookdll=Hook.dll

cflags=$(cflags) -D_UNICODE -DUNICODE

all: $(hookdll) $(keylogger)

clean:
	@if exist $(objs_dir) rmdir /s /q $(objs_dir)

fclean: clean
	@if exist $(keylogger) del $(keylogger)
	@if exist $(hookdll) del $(hookdll)
	@del *.pdb
	@del *.lib
	@del *.exp

$(keylogger): $(objs_dir)\Winkey.obj
	$(link) $(ldebug) $(lflags) $(guiflags) $** -out:$@ $(guilibs) psapi.lib

$(hookdll): $(objs_dir)\Hook.obj
	$(link) $(ldebug) $(lflags) $(dlllflags) $** -out:$@ kernel32.lib user32.lib

{$(srcs_dir)\}.c{$(objs_dir)\}.obj:
	@if not exist $(objs_dir) mkdir $(objs_dir)
	$(cc) $(cdebug) $(cflags) $(cvars) /Fo"$(objs_dir)\\" /c $<