"""Build ExArMoTouch_v2.9.4.py (10-inch touchscreen, Raspberry Pi) from ExArMoRehab_v2.9.3.py.

Same functions and firmware commands as the Rehab GUI; only the layout is adapted:
  * everything (fonts, widths, paddings, canvases, wraplengths) is scaled by S,
    computed from the real screen at start-up (design size 1800x960)
  * fullscreen kiosk window, F11 / Esc toggle, Quit button in Advanced
  * touch-sized slider handles, Linux font fallback (DejaVu Sans)
"""
import os, re, sys

src_path, dst_path = sys.argv[1], sys.argv[2]
s = open(src_path, encoding="utf-8").read()

def R(a, b, cnt=1):
    global s
    n = s.count(a)
    assert n == cnt, (n, a[:100])
    s = s.replace(a, b)

# ---------------------------------------------------------------- header + version
s = s.replace('APP_VERSION = "v2.9.3"', 'APP_VERSION = "v2.9.4-touch"', 1)
assert 'APP_VERSION = "v2.9.4-touch"' in s
s = s.replace('"""', '''"""ExArMo Touch v2.9.4 — the ExArMo Rehab app laid out for a 10-inch touchscreen
(Raspberry Pi 5, 1280x800 or 1024x600). Generated from ExArMoRehab_v2.9.3.py by
make_touch.py: identical functions and firmware commands; all sizes scale to the
screen, fullscreen kiosk (F11 / Esc), Quit under Advanced, bigger slider handles.

''', 1)

# ---------------------------------------------------------------- scale probe
R("import tkinter as tk\n", '''import tkinter as tk
import tkinter.font as tkfont

# ════════════════ TOUCH SCALING (v2.9.4) ════════════════
# Design size of the desktop app is 1800x960. Measure the real screen once and
# scale every font / size / padding by S, so the same layout fits 1280x800
# (S≈0.70) or 1024x600 (S≈0.57). EXARMO_SCALE overrides (e.g. 0.8).
_probe = tk.Tk(); _probe.withdraw()
SCR_W, SCR_H = _probe.winfo_screenwidth(), _probe.winfo_screenheight()
_fams = set(tkfont.families(_probe))
_probe.destroy()
try:
    S = float(os.environ.get("EXARMO_SCALE", "0"))
except ValueError:
    S = 0.0
if S <= 0:
    S = max(0.5, min(1.0, min(SCR_W / 1800.0, SCR_H / 960.0)))
def z(n):
    """scale a design-size pixel/point value to this screen"""
    return max(1, int(round(n * S)))
def zt(n, floor):
    """scale, but never below a touch-usable minimum"""
    return max(floor, z(n))
UIF = next((f for f in ("Segoe UI", "Inter", "Noto Sans", "DejaVu Sans", "Liberation Sans")
            if f in _fams), "TkDefaultFont")
''')
# os must be imported before the probe uses os.environ
if not re.search(r"^import os\b", s.split("import tkinter as tk")[0], re.M):
    s = s.replace("import tkinter as tk\n", "import os\nimport tkinter as tk\n", 1)

# ---------------------------------------------------------------- fonts
s, nf = re.subn(r'\("Segoe UI",\s*(\d+)', lambda m: f'(UIF, z({m.group(1)})', s)
print("fonts scaled:", nf)

# ---------------------------------------------------------------- window
R('''root.geometry("1800x960")
root.minsize(1500, 820)
try:
    root.state("zoomed")          # start maximized on Windows
except tk.TclError:
    pass''',
'''root.geometry(f"{SCR_W}x{SCR_H}+0+0")
root.minsize(min(800, SCR_W), min(480, SCR_H))
_fs = [os.environ.get("EXARMO_WINDOWED", "") == ""]      # fullscreen kiosk by default
def _apply_fs():
    try:
        root.attributes("-fullscreen", _fs[0])
    except tk.TclError:
        pass
def _toggle_fs(e=None):
    _fs[0] = not _fs[0]; _apply_fs()
root.bind("<F11>", _toggle_fs)
root.bind("<Escape>", lambda e: (_fs.__setitem__(0, False), _apply_fs()))
root.after(200, _apply_fs)''')

# ---------------------------------------------------------------- fixed sizes
R("SCREEN_W, SCREEN_H = 1060, 520", "SCREEN_W, SCREEN_H = 1060, 520   # (scaled below, after S is known)")
s = s.replace("SCREEN_W, SCREEN_H = 1060, 520   # (scaled below, after S is known)",
              "SCREEN_W, SCREEN_H = z(1060), z(520)", 1)
R("sidebar = tk.Frame(root, bg=SURFACE, width=300)", "sidebar = tk.Frame(root, bg=SURFACE, width=z(300))")
s, nw = re.subn(r"wraplength=(\d+)", lambda m: f"wraplength=z({m.group(1)})", s); print("wraplengths:", nw)
R("def make_arm_sim(parent, w=430, h=470, modes=()):", "def make_arm_sim(parent, w=430, h=470, modes=()):\n    w, h = z(w), z(h)")
R("    W, H = 170, 260\n", "    W, H = z(170), z(260)\n")
R("gauge = tk.Canvas(inner, width=170, height=260,", "gauge = tk.Canvas(inner, width=z(170), height=z(260),")
R("bar = tk.Frame(inner, bg=accent, height=7, width=76)", "bar = tk.Frame(inner, bg=accent, height=z(7), width=z(76))")

# paddings: padx=N, pady=N and tuples (a, b)
s, n1 = re.subn(r"\b(pad[xy])=(\d+)\b", lambda m: f"{m.group(1)}=z({m.group(2)})", s)
s, n2 = re.subn(r"\b(pad[xy])=\((\d+),\s*(\d+)\)", lambda m: f"{m.group(1)}=(z({m.group(2)}), z({m.group(3)}))", s)
print("paddings:", n1, n2)

# sliders: touch-sized handles (never below 34 x 22 px)
s, n3 = re.subn(r"sliderlength=(\d+)", lambda m: f"sliderlength=zt({m.group(1)}, 34)", s)
s, n4 = re.subn(r"(sliderlength=zt\(\d+, 34\),\s*)width=(\d+)", lambda m: f"{m.group(1)}width=zt({m.group(2)}, 22)", s)
print("sliders:", n3, n4)

# ---------------------------------------------------------------- Quit button (no title bar in fullscreen)
R('''# --- [fw v2.9z] full automatic homing (push down to the bottom stop, then up) ---''',
'''# --- [v2.9.4] Quit + fullscreen toggle (fullscreen has no window title bar) ---
_qrow = tk.Frame(adv_frame, bg=SURFACE)
_qrow.pack(fill="x", padx=z(14), pady=(z(4), 0))
tk.Button(_qrow, text="\\u26f6 Window / Full", command=_toggle_fs,
          bg=SURFACE2, fg=MUTED, relief="flat", bd=0, font=F_SMALL,
          cursor="hand2", highlightthickness=0, pady=z(6)).pack(side="left", fill="x", expand=True, padx=(0, z(4)))
tk.Button(_qrow, text="\\u2715 Quit", command=lambda: on_close(),
          bg=SURFACE2, fg=RED, relief="flat", bd=0, font=F_SMALL,
          cursor="hand2", highlightthickness=0, pady=z(6)).pack(side="left", fill="x", expand=True)

# --- [fw v2.9z] full automatic homing (push down to the bottom stop, then up) ---''')


# ---------------------------------------------------------------- DPI lock
# Tk sizes fonts in POINTS → pixels via the screen DPI. A 10-inch 1280x800 panel
# reports ~150 dpi, which would render every font ~1.5x larger than designed and
# overflow the layout. Lock 1 pt = 1.333 px (96 dpi) so the scale S is the only factor.
R("root = tk.Tk()\n", "root = tk.Tk()\ntry:\n    root.tk.call('tk', 'scaling', 96.0 / 72.0)\nexcept tk.TclError:\n    pass\n")

# ---------------------------------------------------------------- touch sliders: visible, finger-sized handle
s = s.replace("bg=SURFACE, fg=TEXT, troughcolor=SURFACE2", "bg=HANDLE, fg=TEXT, troughcolor=SURFACE2")
s = s.replace("sliderlength=zt(30, 34), width=zt(18, 22)", "sliderlength=zt(30, 46), width=zt(18, 26)")
s = s.replace("sliderlength=zt(26, 34), width=zt(14, 22)", "sliderlength=zt(26, 46), width=zt(14, 26)")
R("UIF = next(", 'HANDLE = "#9fb0d0"     # [v2.9.4] visible slider handle for touch\nUIF = next(')

# ---------------------------------------------------------------- tap-sized small buttons
R("highlightthickness=0, padx=z(6)).pack(side=\"left\")", "highlightthickness=0, padx=z(10), pady=zt(4, 7)).pack(side=\"left\")")
R("cursor=\"hand2\", highlightthickness=0, padx=z(10)).pack(side=\"left\", padx=z(4))",
  "cursor=\"hand2\", highlightthickness=0, padx=z(14), pady=zt(4, 7)).pack(side=\"left\", padx=z(4))")

# ---------------------------------------------------------------- narrow effort column title
R('section_label(inner, "Your effort \\u03c4\\u0302_h \\u2014 live")', 'section_label(inner, "Effort \\u03c4\\u0302_h")')

open(dst_path, "w", encoding="utf-8").write(s)
print("written", dst_path)

# ================================================================ pass 3: small screens (1024x600)
s = open(dst_path, encoding="utf-8").read()

# ---- monospace fonts scale too (Consolas does not exist on the Pi)
s, nm = re.subn(r'\("Consolas",\s*(\d+)', lambda m: f'(MONOF, z({m.group(1)})', s)
print("mono fonts scaled:", nm)
R('HANDLE = "#9fb0d0"', '''MONOF = next((f for f in ("Consolas", "DejaVu Sans Mono", "Liberation Mono", "Noto Sans Mono")
              if f in _fams), "TkFixedFont")
HANDLE = "#9fb0d0"''')

# ---- touch-scrollable settings column: START/STOP stay visible, sliders scroll if the screen is short
R('''def section_label(parent, text):''', '''def scroll_area(parent):
    """[v2.9.4] A column that scrolls (finger-drag, wheel or the wide bar) only
    when its content is taller than the space left on screen. Buttons packed
    after it in the same card always stay visible below it."""
    wrap = tk.Frame(parent, bg=SURFACE)
    wrap.pack(fill="both", expand=True)
    cv = tk.Canvas(wrap, bg=SURFACE, highlightthickness=0, bd=0, width=z(100), height=z(120))
    sb = tk.Scrollbar(wrap, orient="vertical", command=cv.yview, width=zt(16, 24),
                      bg=HANDLE, troughcolor=SURFACE2, activebackground=HANDLE,
                      bd=0, highlightthickness=0, elementborderwidth=0, relief="flat")
    cv.pack(side="left", fill="both", expand=True)
    body = tk.Frame(cv, bg=SURFACE)
    win = cv.create_window(0, 0, window=body, anchor="nw")
    cv.configure(yscrollcommand=sb.set)
    shown = [False]
    def _sync(e=None):
        cw, ch = cv.winfo_width(), cv.winfo_height()
        cv.itemconfigure(win, width=cw)
        bh = body.winfo_reqheight()
        cv.configure(scrollregion=(0, 0, cw, max(bh, ch)))
        need = bh > ch + 2
        if need != shown[0]:
            shown[0] = need
            if need:
                sb.pack(side="right", fill="y", padx=(z(8), 0), before=cv)
            else:
                sb.pack_forget()
                cv.yview_moveto(0)
    body.bind("<Configure>", _sync)
    cv.bind("<Configure>", _sync)
    def _press(e):
        cv.scan_mark(0, e.y_root)
    def _drag(e):
        if shown[0]:
            cv.scan_dragto(0, e.y_root, gain=1)
    def _wheel(e):
        if shown[0]:
            cv.yview_scroll(-1 if (getattr(e, "num", 0) == 4 or getattr(e, "delta", 0) > 0) else 1, "units")
    def _bind_tree(w):
        if not isinstance(w, tk.Scale):          # dragging a slider must move the slider
            w.bind("<ButtonPress-1>", _press, add="+")
            w.bind("<B1-Motion>", _drag, add="+")
        for ev in ("<Button-4>", "<Button-5>", "<MouseWheel>"):
            w.bind(ev, _wheel, add="+")
        for c in w.winfo_children():
            _bind_tree(c)
    root.after(800, lambda: _bind_tree(body))
    return body

def section_label(parent, text):''')

R('section_label(pcard, "Motion settings  (% of homed range)")\n',
  'section_label(pcard, "Motion settings  (% of homed range)")\npbody = scroll_area(pcard)            # [v2.9.4]\n')
s, a = re.subn(r"slider_row\(pcard,", "slider_row(pbody,", s)
R('section_label(acard, "Control settings  (sent to firmware)")\n',
  'section_label(acard, "Control settings  (sent to firmware)")\nabody = scroll_area(acard)            # [v2.9.4]\n')
s, b = re.subn(r"slider_row\(acard,", "slider_row(abody,", s)
R('section_label(rcard, "Control settings  (sent to firmware)")\n',
  'section_label(rcard, "Control settings  (sent to firmware)")\nrbody = scroll_area(rcard)            # [v2.9.4]\n')
s, c = re.subn(r"slider_row\(rcard,", "slider_row(rbody,", s)
R('tk.Label(rcard, text="How it works', 'tk.Label(rbody, text="How it works')
print("sliders moved into scroll areas:", a, b, c)
assert (a, b, c) == (7, 8, 5)

# ---- Advanced opens as a floating panel beside the sidebar (sidebar keeps all nav buttons)
R('adv_frame = tk.Frame(sidebar, bg=SURFACE)\n', '''adv_frame = tk.Frame(root, bg=SURFACE, highlightthickness=2,
                     highlightbackground=BORDER, highlightcolor=BORDER)   # [v2.9.4] floating panel
_advh = tk.Frame(adv_frame, bg=SURFACE)
_advh.pack(fill="x", padx=z(12), pady=(z(8), z(4)))
tk.Label(_advh, text="\\u2699  Advanced", bg=SURFACE, fg=TEXT, font=F_LABEL).pack(side="left")
tk.Button(_advh, text="\\u2715", command=lambda: toggle_adv(), bg=SURFACE2, fg=MUTED,
          relief="flat", bd=0, font=F_LABEL, padx=zt(10, 14), pady=zt(2, 4),
          cursor="hand2", highlightthickness=0).pack(side="right")
''')
R('''        adv_frame.pack(fill="x", after=adv_toggle)''', '''        root.update_idletasks()
        _h = adv_frame.winfo_reqheight()
        _y = adv_toggle.winfo_rooty() - root.winfo_rooty()
        _y = max(z(6), min(_y, root.winfo_height() - _h - z(6)))
        adv_frame.place(x=sidebar.winfo_width() + z(6), y=_y, width=max(280, z(380)))
        adv_frame.lift()''')
R('''        adv_frame.pack_forget()''', '''        adv_frame.place_forget()''')

# ---- readable minimum font size on 1024x600 (never below 9 pt)
s, nfl = re.subn(r"\((UIF|MONOF), z\((\d+)\)", lambda m: f"({m.group(1)}, zt({m.group(2)}, 9)", s)
print("font floors:", nfl)
# ---- finger-sized buttons in the Advanced panel
s = s.replace("cursor=\"hand2\", highlightthickness=0, pady=z(6)).pack(side=\"left\"",
              "cursor=\"hand2\", highlightthickness=0, pady=zt(6, 10)).pack(side=\"left\"")
R('cursor="hand2", highlightthickness=0, pady=z(4)).pack(fill="x", padx=z(14), pady=(z(4), z(0)))',
  'cursor="hand2", highlightthickness=0, pady=zt(4, 10)).pack(fill="x", padx=z(14), pady=(z(4), z(0)))')
R('text=f"ELBOW REHABILITATION \\u00b7 {APP_VERSION}"', 'text=f"REHAB \\u00b7 {APP_VERSION}"')
# ---- Raspberry Pi: put the board's USB port (/dev/ttyACM*, STM32 VID 0483) first, hide built-in UARTs
R('''ports = [p.device for p in serial.tools.list_ports.comports()] or ["\\u2014"]''',
'''def _sorted_ports():
    """[v2.9.4] the MKS board (USB CDC, /dev/ttyACM*) first; Pi built-in UARTs last"""
    def rank(q):
        d = q.device
        if getattr(q, "vid", None) == 0x0483 or "ACM" in d: return 0
        if "USB" in d or d.upper().startswith("COM"): return 1
        return 2
    return [q.device for q in sorted(serial.tools.list_ports.comports(), key=rank)]
ports = _sorted_ports() or ["\\u2014"]''')
R('''    for p in [q.device for q in serial.tools.list_ports.comports()] or ["\\u2014"]:''',
  '''    for p in _sorted_ports() or ["\\u2014"]:''')
R('''port_menu["menu"].config(bg=SURFACE2, fg=TEXT, bd=0)''',
  '''port_menu["menu"].config(bg=SURFACE2, fg=TEXT, bd=0, font=F_LABEL)   # [v2.9.4] finger-sized list''')
open(dst_path, "w", encoding="utf-8").write(s)
print("pass 3 written")

# ================================================================ pass 4: start-up page (connect + homing first)
s = open(dst_path, encoding="utf-8").read()
SETUP_BLOCK = '# ════════════════ [v2.9.4] START-UP PAGE: connect + homing before any therapy mode ════════════════\n# Shown full-screen when the app opens (covers the sidebar). The board is found and connected\n# automatically (USB /dev/ttyACM*, STM32); then the arm must be homed. Only after EV,HOMED does\n# "Continue" unlock the therapy modes. If the board is unplugged or the range is lost later, the\n# modes lock again and this page comes back.  EXARMO_NOSETUP=1 skips it (desk testing).\n_setup_on = [False]\n_setup_skip = os.environ.get("EXARMO_NOSETUP", "") != ""\n_auto_tries = [0]\n\nsetup_frame = tk.Frame(root, bg=BG)\n_su = tk.Frame(setup_frame, bg=BG)\n_su.pack(fill="both", expand=True, padx=z(40), pady=z(26))\n\n_sut = tk.Frame(_su, bg=BG)\n_sut.pack(fill="x")\ntk.Label(_sut, text="Ex", bg=BG, fg=AMBER, font=F_TITLE).pack(side="left")\ntk.Label(_sut, text="ArMo  ·  Get ready", bg=BG, fg=TEXT, font=F_TITLE).pack(side="left")\ntk.Button(_sut, text="✕ Quit", command=lambda: on_close(), bg=SURFACE2, fg=RED,\n          relief="flat", bd=0, font=F_SMALL, padx=zt(14, 12), pady=zt(6, 10),\n          cursor="hand2", highlightthickness=0).pack(side="right")\ntk.Button(_sut, text="⛶", command=_toggle_fs, bg=SURFACE2, fg=MUTED,\n          relief="flat", bd=0, font=F_SMALL, padx=zt(14, 12), pady=zt(6, 10),\n          cursor="hand2", highlightthickness=0).pack(side="right", padx=(z(0), z(8)))\ntk.Label(_su, text="Connect the device and find the arm\'s movement range — therapy modes unlock after homing",\n         bg=BG, fg=MUTED, font=F_LABEL).pack(anchor="w", pady=(z(2), z(16)))\n\n_surow = tk.Frame(_su, bg=BG)\n_surow.pack(fill="both", expand=True)\n_sucol = tk.Frame(_surow, bg=BG)\n_sucol.pack(side="left", fill="both", expand=True, padx=(z(0), z(16)))\n\ndef _step_card(parent, num, title):\n    o, i = card(parent)\n    hdr = tk.Frame(i, bg=SURFACE)\n    hdr.pack(fill="x", pady=(z(0), z(10)))\n    badge = tk.Label(hdr, text=str(num), bg=SURFACE2, fg=MUTED, width=2,\n                     font=(UIF, zt(18, 11), "bold"), padx=z(4), pady=z(2))\n    badge.pack(side="left", padx=(z(0), z(12)))\n    tk.Label(hdr, text=title, bg=SURFACE, fg=TEXT, font=F_H1).pack(side="left")\n    return o, i, badge\n\n# ---- step 1: connect\n_c1o, _c1, _b1 = _step_card(_sucol, 1, "Connect the device")\n_c1o.pack(fill="x", pady=(z(0), z(14)))\nsu_conn = tk.Label(_c1, text="●  Looking for the ExArMo board…", bg=SURFACE, fg=MUTED,\n                   font=F_LABEL, anchor="w", justify="left", wraplength=z(700))\nsu_conn.pack(fill="x", pady=(z(0), z(10)))\n_c1r = tk.Frame(_c1, bg=SURFACE)\n_c1r.pack(fill="x")\nsu_port_menu = tk.OptionMenu(_c1r, port_var, *ports)\nsu_port_menu.config(bg=SURFACE2, fg=TEXT, relief="flat", bd=0, highlightthickness=0,\n                    font=F_LABEL, activebackground=BORDER, activeforeground=TEXT,\n                    pady=zt(8, 10))\nsu_port_menu["menu"].config(bg=SURFACE2, fg=TEXT, bd=0, font=F_LABEL)\nsu_port_menu.pack(side="left", fill="x", expand=True, padx=(z(0), z(8)))\ndef su_refresh_ports():\n    pl = _sorted_ports() or ["—"]\n    for om in (port_menu, su_port_menu):\n        m = om["menu"]\n        m.delete(0, "end")\n        for p in pl:\n            m.add_command(label=p, command=lambda v=p: port_var.set(v))\n    if port_var.get() not in pl:\n        port_var.set(pl[0])\ntk.Button(_c1r, text="↻", command=su_refresh_ports, bg=SURFACE2, fg=MUTED,\n          relief="flat", bd=0, font=F_BTN, padx=zt(16, 14), pady=zt(8, 8),\n          cursor="hand2", highlightthickness=0).pack(side="left", padx=(z(0), z(8)))\nsu_conn_btn = flat_button(_c1r, "Connect", CYAN, "#062430",\n                          lambda: connect(port_var.get()))\nsu_conn_btn.pack(side="left")\n\n# ---- step 2: homing\n_c2o, _c2, _b2 = _step_card(_sucol, 2, "Find the movement range (homing)")\n_c2o.pack(fill="both", expand=True)\ntk.Label(_c2, text="Put the arm down on the bottom stop and tap  0 here,  then tap  Find limits.\\n"\n                   "The arm lifts slowly to the top stop and parks. Keep hands clear.",\n         bg=SURFACE, fg=MUTED, font=F_LABEL, justify="left", anchor="w",\n         wraplength=z(760)).pack(fill="x", pady=(z(0), z(12)))\n_c2r = tk.Frame(_c2, bg=SURFACE)\n_c2r.pack(fill="x")\nsu_zero_btn = flat_button(_c2r, "0 here", AMBER, "#2e2005",\n                          lambda: (stop_all_modes(), send("Z")))\nsu_zero_btn.pack(side="left", padx=(z(0), z(10)))\nsu_home_btn = flat_button(_c2r, "⌂  Find limits", GREEN, "#06251d",\n                          lambda: (stop_all_modes(), send("H")))\nsu_home_btn.pack(side="left", fill="x", expand=True, padx=(z(0), z(10)))\nsu_stop_btn = flat_button(_c2r, "■  STOP", RED, "#2b070d", lambda: send("O0"))\nsu_stop_btn.pack(side="left")\ntk.Button(_c2, text="Full automatic homing (push down, then up) — H1",\n          command=lambda: (stop_all_modes(), send("H1")),\n          bg=SURFACE2, fg=MUTED, relief="flat", bd=0, font=F_SMALL,\n          activebackground=BORDER, activeforeground=TEXT,\n          pady=zt(6, 10), cursor="hand2", highlightthickness=0).pack(fill="x", pady=(z(10), z(0)))\nsu_home = tk.Label(_c2, text="●  Not homed", bg=SURFACE, fg=MUTED, font=F_LABEL,\n                   anchor="w", justify="left", wraplength=z(760))\nsu_home.pack(fill="x", pady=(z(12), z(0)))\n_c2a = tk.Frame(_c2, bg=SURFACE)\n_c2a.pack(side="bottom", fill="x")\nsu_angle = tk.Label(_c2a, text="+0\\u00b0", bg=SURFACE, fg=CYAN, font=(MONOF, zt(44, 22), "bold"))\nsu_angle.pack(side="left")\nsu_state = tk.Label(_c2a, text="IDLE", bg=SURFACE, fg=MUTED, font=F_H1)\nsu_state.pack(side="left", padx=(z(18), z(0)), pady=(z(10), z(0)))\ntk.Label(_c2a, text="arm angle", bg=SURFACE, fg=FAINT, font=F_SMALL).pack(side="right", anchor="s")\n\n# ---- right column: live arm + continue\n_suright = tk.Frame(_surow, bg=BG)\n_suright.pack(side="left", fill="y")\nmake_arm_sim(_suright, w=380, h=380, modes=("setup",)).pack(fill="x")\nsu_go = tk.Button(_suright, text="Continue  →\\nchoose therapy", command=lambda: hide_setup(),\n                  bg=SURFACE2, fg=FAINT, relief="flat", bd=0, state="disabled",\n                  disabledforeground=FAINT, font=F_BTN, pady=z(16),\n                  cursor="hand2", highlightthickness=0)\nsu_go.pack(fill="both", expand=True, pady=(z(14), z(0)))\n\ndef _connected():\n    return ser is not None and getattr(ser, "is_open", False)\n\ndef show_setup(reason=None):\n    global mode\n    if _setup_skip:\n        return\n    if mode not in ("menu", "setup"):\n        stop_all_modes()\n    mode = "setup"\n    update_telem_rate()\n    if reason:\n        set_status(su_home, reason, AMBER)\n    _setup_on[0] = True\n    setup_frame.place(x=0, y=0, relwidth=1, relheight=1)\n    setup_frame.lift()\n\ndef hide_setup():\n    if not (_connected() and homed):\n        return\n    _setup_on[0] = False\n    setup_frame.place_forget()\n    show_menu()\n\ndef _need_setup():\n    """therapy pages need a connected, homed device"""\n    if _setup_skip or (_connected() and homed):\n        return False\n    show_setup()\n    return True\n\ndef _setup_tick():\n    if not running:\n        return\n    con = _connected()\n    st = T["state"]\n    # step 1\n    if con:\n        su_conn.config(text="●  Connected — " + conn_status.cget("text").lstrip("● ").strip(),\n                       fg=GREEN if fw_ok is not False else AMBER)\n        _b1.config(text="✓", bg=GREEN, fg="#06251d")\n        su_conn_btn.config(text="Reconnect")\n    else:\n        _b1.config(text="1", bg=SURFACE2, fg=MUTED)\n        su_conn_btn.config(text="Connect")\n        txt = conn_status.cget("text")\n        if "disconnected" in txt or txt.strip() in ("", "●"):\n            su_conn.config(text="●  Looking for the ExArMo board… plug in the USB cable and power",\n                           fg=MUTED)\n        else:\n            su_conn.config(text=txt, fg=RED)\n        # auto-connect to the board\'s own USB port (never to the Pi\'s built-in UARTs)\n        if _setup_on[0] and _auto_tries[0] % 4 == 0:\n            for q in serial.tools.list_ports.comports():\n                if getattr(q, "vid", None) == 0x0483 or "ACM" in q.device:\n                    port_var.set(q.device)\n                    su_refresh_ports()\n                    connect(q.device)\n                    break\n        _auto_tries[0] += 1\n    # step 2\n    for b, bg_on, fg_on in ((su_zero_btn, AMBER, "#2e2005"), (su_home_btn, GREEN, "#06251d"),\n                            (su_stop_btn, RED, "#2b070d")):\n        if con:\n            if b.cget("state") == "disabled":\n                b.config(state="normal", bg=bg_on, fg=fg_on)\n        else:\n            b.config(state="disabled", bg=SURFACE2, disabledforeground=FAINT)\n    su_angle.config(text=f"{math.degrees(T[\'theta\']):+.0f}\\u00b0" if con else "\\u2014")\n    su_state.config(text=STATE_NAMES.get(st, "?") if con else "", fg=STATE_COLORS.get(st, MUTED))\n    if not con:\n        su_home.config(text="●  Connect first", fg=FAINT)\n    elif st in (1, 2, 3):\n        su_home.config(text=f"●  {STATE_NAMES.get(st, \'\')} — homing…  "\n                            f"{math.degrees(T[\'theta\']):+.0f}°", fg=CYAN)\n    else:\n        su_home.config(text=home_status.cget("text"), fg=home_status.cget("fg"))\n    _b2.config(text="✓" if homed else "2",\n               bg=GREEN if homed else SURFACE2, fg="#06251d" if homed else MUTED)\n    ready = con and homed and st not in (1, 2, 3)\n    su_go.config(state="normal" if ready else "disabled",\n                 bg=GREEN if ready else SURFACE2, fg="#06251d" if ready else FAINT)\n    # lock the modes again if the board is lost / the range is no longer valid\n    if not _setup_on[0] and not _setup_skip and not (con and homed):\n        show_setup("Connection lost — reconnect" if not con else\n                   "Range lost — find limits again")\n    root.after(250, _setup_tick)\n\n'
R("# ---------- start everything ----------\n", SETUP_BLOCK + "# ---------- start everything ----------\n")
for fn in ("show_passive", "show_assistive", "show_resistive", "show_game"):
    R(f"def {fn}():\n    global mode\n", f"def {fn}():\n    global mode\n    if _need_setup():           # [v2.9.4] home first\n        return\n")
R("update_checklist()\n\nthreading.Thread", "update_checklist()\nshow_setup()\n_setup_tick()\n\nthreading.Thread")
# arm view: 'table' caption away from the big angle readout (overlapped on small screens)
R('c.create_text(w - 14, ty + 26, text="table", fill=FAINT, font=F_SMALL, anchor="ne")',
  'c.create_text(ex + 40, ty + 12, text="table", fill=FAINT, font=F_SMALL, anchor="nw")')
open(dst_path, "w", encoding="utf-8").write(s)
print("pass 4 written")

UI_KIT = '# ════════════════ [v2.9.4] ROUNDED UI KIT (touch look) ════════════════\n# Tk has no rounded widgets, so cards, buttons and sliders are drawn here:\n# anti-aliased shapes rendered with Pillow (python3-pil + python3-pil.imagetk on the Pi);\n# without Pillow the same widgets fall back to plain canvas shapes.\ntry:\n    from PIL import Image, ImageDraw, ImageTk\n    _HAVE_PIL = True\nexcept Exception:\n    _HAVE_PIL = False\n\nRADIUS   = zt(18, 10)      # cards\nRADIUS_B = zt(12, 8)       # buttons\nKNOB_C   = "#eef2fb"       # slider knob\n_img_cache = {}\n_font_cache = {}\n\ndef _rgb(c):\n    c = c.lstrip("#")\n    return tuple(int(c[i:i + 2], 16) for i in (0, 2, 4))\n\ndef _rr_photo(w, h, r, fill, bg, outline=None, ow=1):\n    """anti-aliased rounded rectangle (or circle) as a cached PhotoImage"""\n    w, h = max(2, int(w)), max(2, int(h))\n    r = max(0, min(int(r), w // 2, h // 2))\n    key = (w, h, r, fill, bg, outline, ow)\n    im = _img_cache.get(key)\n    if im is not None:\n        return im\n    if len(_img_cache) > 800:\n        _img_cache.clear()          # widgets keep their own reference, so this is safe\n    k = 3\n    big = Image.new("RGB", (w * k, h * k), _rgb(bg))\n    d = ImageDraw.Draw(big)\n    if outline:\n        d.rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=r * k, fill=_rgb(outline))\n        o = ow * k\n        d.rounded_rectangle((o, o, w * k - 1 - o, h * k - 1 - o), radius=max(0, r * k - o),\n                            fill=_rgb(fill))\n    else:\n        d.rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=r * k, fill=_rgb(fill))\n    im = ImageTk.PhotoImage(big.resize((w, h), Image.LANCZOS))\n    _img_cache[key] = im\n    return im\n\ndef _rr_points(x1, y1, x2, y2, r):\n    return [x1 + r, y1, x1 + r, y1, x2 - r, y1, x2 - r, y1, x2, y1, x2, y1 + r, x2, y1 + r,\n            x2, y2 - r, x2, y2 - r, x2, y2, x2 - r, y2, x2 - r, y2, x1 + r, y2, x1 + r, y2,\n            x1, y2, x1, y2 - r, x1, y2 - r, x1, y1 + r, x1, y1 + r, x1, y1]\n\ndef _rr_draw(c, x, y, w, h, r, fill, outline=None, tag="rr", keep=None):\n    """draw a rounded rect on canvas c at (x, y); keep = list that holds the image ref"""\n    if w < 2 or h < 2:\n        return\n    if _HAVE_PIL:\n        im = _rr_photo(w, h, r, fill, tk.Canvas.cget(c, "bg"), outline)\n        if keep is not None:\n            keep.append(im)\n        c.create_image(int(x), int(y), image=im, anchor="nw", tags=tag)\n    else:\n        r = max(1, min(r, w // 2, h // 2))\n        c.create_polygon(_rr_points(x, y, x + w - 1, y + h - 1, r), smooth=True,\n                         fill=fill, outline=outline or fill, tags=tag)\n\ndef _rr_bg(c, color, r=None):\n    """rounded dark background for the live canvases (arm view, gauge, game)"""\n    w = c.winfo_width() if c.winfo_width() > 2 else int(float(c.cget("width")))\n    h = c.winfo_height() if c.winfo_height() > 2 else int(float(c.cget("height")))\n    c._bgkeep = []\n    _rr_draw(c, 0, 0, w, h, RADIUS if r is None else r, color, tag="rrbg", keep=c._bgkeep)\n    c.tag_lower("rrbg")\n\ndef _font(f):\n    key = repr(f)\n    if key not in _font_cache:\n        _font_cache[key] = tkfont.Font(root=root, font=f)\n    return _font_cache[key]\n\n\nclass RoundBox(tk.Frame):\n    """A frame with a rounded, anti-aliased background. config(bg=…) changes the\n    FILL colour; the corners keep the parent\'s colour. Pack children with\n    padx/pady = .inset so their square corners stay inside the rounded shape."""\n    def __init__(self, parent, fill=SURFACE, outline=BORDER, radius=None, **kw):\n        self._pbg = parent.cget("bg")\n        tk.Frame.__init__(self, parent, bg=self._pbg if _HAVE_PIL else fill, **kw)\n        self._fill, self._outline = fill, outline\n        self._r = RADIUS if radius is None else radius\n        self.inset = (int(self._r * 0.3) + 2) if _HAVE_PIL else 0\n        self._img = None\n        if _HAVE_PIL:\n            self._bgl = tk.Label(self, bd=0, highlightthickness=0, bg=self._pbg)\n            self._bgl.place(x=0, y=0, relwidth=1, relheight=1)\n            self._bgl.lower()\n            self.bind("<Configure>", self._redraw, add="+")\n    def _redraw(self, e=None):\n        w, h = self.winfo_width(), self.winfo_height()\n        if w < 4 or h < 4:\n            return\n        self._img = _rr_photo(w, h, self._r, self._fill, self._pbg, self._outline)\n        self._bgl.config(image=self._img)\n    def set_fill(self, color):\n        if color != self._fill:\n            self._fill = color\n            if _HAVE_PIL:\n                self._redraw()\n            else:\n                tk.Frame.configure(self, bg=color)\n    def configure(self, cnf=None, **kw):\n        if cnf:\n            kw.update(cnf)\n        for k in ("bg", "background"):\n            if k in kw:\n                self.set_fill(kw.pop(k))\n        if kw:\n            return tk.Frame.configure(self, **kw)\n    config = configure\n    def __setitem__(self, k, v):\n        self.configure({k: v})\n\n\nclass RButton(tk.Canvas):\n    """Rounded touch button with the tk.Button options this app uses\n    (text, command, bg, fg, font, padx, pady, width, state, active*/disabled*)."""\n    _OPTS = ("text", "command", "bg", "fg", "font", "padx", "pady", "width", "state",\n             "activebackground", "activeforeground", "disabledforeground", "anchor")\n    def __init__(self, parent, text="", command=None, bg=SURFACE2, fg=TEXT, font=None,\n                 padx=None, pady=None, width=None, state="normal", activebackground=None,\n                 activeforeground=None, disabledforeground=None, anchor="center",\n                 radius=None, cursor="hand2", textvariable=None, suffix="", **_ignored):\n        self._pbg = parent.cget("bg")\n        tk.Canvas.__init__(self, parent, bg=self._pbg, highlightthickness=0, bd=0,\n                           cursor=cursor or "")\n        self._o = dict(text=text, command=command, bg=bg, fg=fg, font=font or F_BTN,\n                       padx=z(16) if padx is None else padx, pady=z(8) if pady is None else pady,\n                       width=width, state=state, activebackground=activebackground,\n                       activeforeground=activeforeground,\n                       disabledforeground=disabledforeground or FAINT, anchor=anchor)\n        self._r = RADIUS_B if radius is None else radius\n        self._hover = self._down = False\n        self._keep = []\n        self._suffix = suffix\n        if textvariable is not None:\n            self._tv = textvariable\n            self._o["text"] = textvariable.get() + suffix\n            textvariable.trace_add("write", lambda *a: self.configure(text=self._tv.get() + self._suffix))\n        self._req()\n        self.bind("<Configure>", lambda e: self._draw())\n        self.bind("<Enter>", lambda e: self._set(hover=True), add="+")\n        self.bind("<Leave>", lambda e: self._set(hover=False, down=False), add="+")\n        self.bind("<ButtonPress-1>", lambda e: self._set(down=True), add="+")\n        self.bind("<ButtonRelease-1>", self._release, add="+")\n    def _req(self):\n        f = _font(self._o["font"])\n        lines = str(self._o["text"]).split("\\n")\n        tw = (f.measure("0") * self._o["width"]) if self._o["width"] else max(f.measure(l) for l in lines)\n        th = f.metrics("linespace") * len(lines)\n        tk.Canvas.configure(self, width=int(tw + 2 * self._o["padx"]),\n                            height=int(th + 2 * self._o["pady"]))\n    def _set(self, hover=None, down=None):\n        if hover is not None:\n            self._hover = hover\n        if down is not None:\n            self._down = down\n        self._draw()\n    def _draw(self):\n        w, h = self.winfo_width(), self.winfo_height()\n        if w < 4 or h < 4:\n            w, h = int(float(self.cget_canvas("width"))), int(float(self.cget_canvas("height")))\n        o = self._o\n        self.delete("all")\n        self._keep = []\n        dis = o["state"] == "disabled"\n        fill, fg = o["bg"], o["fg"]\n        if dis:\n            fg = o["disabledforeground"]\n        elif self._down:\n            fill = o["activebackground"] or _lighten(o["bg"], 34)\n            fg = o["activeforeground"] or o["fg"]\n        elif self._hover:\n            fill = _lighten(o["bg"], 12)\n        if fill != self._pbg:\n            _rr_draw(self, 0, 0, w, h, self._r, fill, keep=self._keep)\n        if o["anchor"] == "w":\n            self.create_text(o["padx"], h // 2, text=o["text"], fill=fg, font=o["font"],\n                             anchor="w", justify="left")\n        else:\n            self.create_text(w // 2, h // 2, text=o["text"], fill=fg, font=o["font"],\n                             anchor="center", justify="center")\n    def _release(self, e):\n        was = self._down\n        self._down = False\n        self._draw()\n        if (was and self._o["state"] != "disabled" and self._o["command"]\n                and 0 <= e.x < self.winfo_width() and 0 <= e.y < self.winfo_height()):\n            self._o["command"]()\n    def cget_canvas(self, k):\n        return tk.Canvas.cget(self, k)\n    def configure(self, cnf=None, **kw):\n        if cnf:\n            kw.update(cnf)\n        if not kw:\n            return tk.Canvas.configure(self)\n        relayout = False\n        for k in list(kw):\n            kk = {"background": "bg", "foreground": "fg"}.get(k, k)\n            if kk in self._OPTS:\n                v = kw.pop(k)\n                if kk in ("text", "font", "width", "padx", "pady") and self._o.get(kk) != v:\n                    relayout = True\n                self._o[kk] = v\n            elif k in ("relief", "bd", "borderwidth", "highlightthickness", "image", "compound"):\n                kw.pop(k)\n        if kw:\n            tk.Canvas.configure(self, **kw)\n        if relayout:\n            self._req()\n        self._draw()\n    config = configure\n    def cget(self, k):\n        kk = {"background": "bg", "foreground": "fg"}.get(k, k)\n        if kk in self._o:\n            return self._o[kk]\n        return tk.Canvas.cget(self, k)\n    def __setitem__(self, k, v):\n        self.configure({k: v})\n    def __getitem__(self, k):\n        return self.cget(k)\n\n\nclass RScale(tk.Canvas):\n    """Touch slider: rounded track, coloured fill up to the value, round knob.\n    Same behaviour as tk.Scale for this app: variable, resolution, command(str)\n    on user changes only (not when the variable is set from code)."""\n    def __init__(self, parent, from_=0.0, to=1.0, resolution=0.01, variable=None,\n                 command=None, activebackground=None, **_ignored):\n        self._pbg = parent.cget("bg")\n        self.K = zt(28, 30)                   # knob diameter (finger-sized)\n        self.T = max(8, self.K // 3)          # track thickness\n        tk.Canvas.__init__(self, parent, bg=self._pbg, highlightthickness=0, bd=0,\n                           height=self.K + 2, width=z(200), cursor="hand2")\n        self.lo, self.hi, self.res = float(from_), float(to), float(resolution)\n        self.dec = max(0, -int(math.floor(math.log10(self.res)))) if self.res > 0 else 3\n        self.var = variable if variable is not None else tk.DoubleVar(value=self.lo)\n        self.cmd = command\n        self.accent = activebackground or CYAN\n        self._drag = False\n        self._keep = []\n        self.var.trace_add("write", lambda *a: self._draw())\n        self.bind("<Configure>", lambda e: self._draw())\n        self.bind("<ButtonPress-1>", self._press)\n        self.bind("<B1-Motion>", self._move)\n        self.bind("<ButtonRelease-1>", self._up)\n    def _geom(self):\n        w = max(self.winfo_width(), 40)\n        m = self.K // 2 + 2\n        return w, m, w - 2 * m\n    def _value(self):\n        try:\n            v = float(self.var.get())\n        except (tk.TclError, ValueError):\n            v = self.lo\n        return min(self.hi, max(self.lo, v))\n    def _draw(self):\n        w, m, span = self._geom()\n        h = self.K + 2\n        cy = h // 2\n        v = self._value()\n        frac = 0.0 if self.hi == self.lo else (v - self.lo) / (self.hi - self.lo)\n        x = m + frac * span\n        self.delete("all")\n        self._keep = []\n        T = self.T\n        _rr_draw(self, m - T // 2, cy - T // 2, span + T, T, T // 2, SURFACE2, keep=self._keep)\n        fw = int(x - m) + T\n        if fw > T:\n            _rr_draw(self, m - T // 2, cy - T // 2, fw, T, T // 2, self.accent, keep=self._keep)\n        K = self.K\n        if _HAVE_PIL:\n            im = _rr_photo(K, K, K // 2, KNOB_C, self._pbg, self.accent if self._drag else BORDER, 3)\n            self._keep.append(im)\n            self.create_image(int(x), cy, image=im, anchor="center")\n        else:\n            self.create_oval(x - K / 2, cy - K / 2, x + K / 2, cy + K / 2, fill=KNOB_C,\n                             outline=self.accent if self._drag else BORDER, width=2)\n    def _set_from_x(self, ex):\n        w, m, span = self._geom()\n        frac = min(1.0, max(0.0, (ex - m) / span)) if span > 0 else 0.0\n        v = self.lo + frac * (self.hi - self.lo)\n        if self.res > 0:\n            v = self.lo + round((v - self.lo) / self.res) * self.res\n        v = round(min(self.hi, max(self.lo, v)), self.dec)\n        if abs(v - self._value()) > 1e-12:\n            self.var.set(v)\n            if self.cmd:\n                self.cmd(f"{v:g}")\n    def _press(self, e):\n        self._drag = True\n        self._set_from_x(e.x)\n        self._draw()\n    def _move(self, e):\n        self._set_from_x(e.x)\n    def _up(self, e):\n        self._drag = False\n        self._draw()\n\n\nclass RScroll(tk.Canvas):\n    """slim rounded scroll bar for touch: drag the thumb or tap the track"""\n    def __init__(self, parent, target):\n        self._pbg = parent.cget("bg")\n        tk.Canvas.__init__(self, parent, bg=self._pbg, highlightthickness=0, bd=0,\n                           width=zt(12, 14), cursor="hand2")\n        self.t = target\n        self.f = (0.0, 1.0)\n        self._keep = []\n        self._grab = None\n        self.bind("<Configure>", lambda e: self._draw())\n        self.bind("<ButtonPress-1>", self._press)\n        self.bind("<B1-Motion>", self._move)\n        self.bind("<ButtonRelease-1>", lambda e: setattr(self, "_grab", None))\n    def set(self, first, last):\n        self.f = (float(first), float(last))\n        self._draw()\n    def _draw(self):\n        w, h = self.winfo_width(), self.winfo_height()\n        if w < 4 or h < 8:\n            return\n        self.delete("all")\n        self._keep = []\n        _rr_draw(self, 0, 0, w, h, w // 2, SURFACE2, keep=self._keep)\n        a, b = self.f\n        y1, y2 = int(a * h), max(int(b * h), int(a * h) + w * 2)\n        _rr_draw(self, 0, y1, w, min(h, y2) - y1, w // 2, HANDLE, keep=self._keep)\n    def _press(self, e):\n        h = max(1, self.winfo_height())\n        a, b = self.f\n        if a * h <= e.y <= b * h:\n            self._grab = (e.y, a)\n        else:\n            self.t.yview_moveto(max(0.0, e.y / h - (b - a) / 2))\n            self._grab = (e.y, self.f[0])\n    def _move(self, e):\n        if self._grab:\n            h = max(1, self.winfo_height())\n            y0, a0 = self._grab\n            self.t.yview_moveto(max(0.0, a0 + (e.y - y0) / h))\n'

# ================================================================ pass 5: rounded look (cards, buttons, sliders, fonts)
s = open(dst_path, encoding="utf-8").read()
KIT = UI_KIT

# fonts: modern UI font first (Inter on the Pi via fonts-inter, Piboto = Pi OS default)
R('''UIF = next((f for f in ("Segoe UI", "Inter", "Noto Sans", "DejaVu Sans", "Liberation Sans")''',
  '''UIF = next((f for f in ("Inter", "Piboto", "Segoe UI", "Noto Sans", "DejaVu Sans", "Liberation Sans")''')
R('''MONOF = next((f for f in ("Consolas", "DejaVu Sans Mono", "Liberation Mono", "Noto Sans Mono")''',
  '''MONOF = next((f for f in ("JetBrains Mono", "Cascadia Mono", "Consolas", "DejaVu Sans Mono", "Liberation Mono")''')

# the kit goes in front of flat_button (needs colours, z(), _lighten, root)
R("def flat_button(parent, text, bg, fg, command, width=None):\n", KIT + "\n\ndef flat_button(parent, text, bg, fg, command, width=None):\n")
R('''    b = tk.Button(parent, text=text, command=command, bg=bg, fg=fg,
                  activebackground=_lighten(bg), activeforeground=fg,
                  relief="flat", bd=0, font=F_BTN, padx=z(28), pady=z(14),
                  cursor="hand2", highlightthickness=0)
    if width:
        b.config(width=width)
    b.bind("<Enter>", lambda e: b.config(bg=_lighten(b.cget("bg"))))
    b.bind("<Leave>", lambda e: b.config(bg=bg))
    return b''', '''    return RButton(parent, text=text, command=command, bg=bg, fg=fg,
                   activebackground=_lighten(bg, 34), activeforeground=fg,
                   font=F_BTN, padx=z(28), pady=z(14), width=width)''')
s, nb = re.subn(r"\btk\.Button\(", "RButton(", s); print("buttons -> RButton:", nb)
s, ns = re.subn(r"\btk\.Scale\(", "RScale(", s); print("sliders -> RScale:", ns)
R("if not isinstance(w, tk.Scale):", "if not isinstance(w, (tk.Scale, RScale)):")

# cards
R('''def card(parent):
    outer = tk.Frame(parent, bg=BORDER, padx=z(1), pady=z(1))
    inner = tk.Frame(outer, bg=SURFACE, padx=z(26), pady=z(22))
    inner.pack(fill="both", expand=True)
    return outer, inner''', '''def card(parent):
    outer = RoundBox(parent)
    inner = tk.Frame(outer, bg=SURFACE, padx=max(0, z(26) - outer.inset),
                     pady=max(0, z(22) - outer.inset))
    inner.pack(fill="both", expand=True, padx=outer.inset, pady=outer.inset)
    return outer, inner''')
R('''    outer = tk.Frame(parent, bg=BORDER, padx=z(1), pady=z(1))
    inner = tk.Frame(outer, bg=SURFACE, padx=z(30), pady=z(26), cursor="hand2")
    inner.pack(fill="both", expand=True)''', '''    outer = RoundBox(parent)
    inner = tk.Frame(outer, bg=SURFACE, padx=max(0, z(30) - outer.inset),
                     pady=max(0, z(26) - outer.inset), cursor="hand2")
    inner.pack(fill="both", expand=True, padx=outer.inset, pady=outer.inset)''')
R("    widgets = (inner, t, d, go)\n", "    widgets = (outer, inner, t, d, go)\n")

# sidebar navigation = rounded pills
R('''    b = tk.Label(sidebar, text=text, bg=SURFACE, fg=MUTED, font=(UIF, zt(17, 9)),
                 anchor="w", padx=z(28), pady=z(14), cursor="hand2")
    b.pack(fill="x")
    b.bind("<Button-1>", lambda e: command())
    b.bind("<Enter>", lambda e: b.config(bg=SURFACE2) if nav_active[0] != key else None)
    b.bind("<Leave>", lambda e: b.config(bg=SURFACE) if nav_active[0] != key else None)''',
'''    b = RButton(sidebar, text=text, command=command, bg=SURFACE, fg=MUTED,
                font=(UIF, zt(17, 9), "bold"), anchor="w", padx=z(20), pady=z(12))
    b.pack(fill="x", padx=z(10), pady=z(1))''')

# checklist pills on Home
R('''    l = tk.Label(_chk, text=text, bg=SURFACE, fg=MUTED,
                 font=(UIF, zt(14, 9), "bold"), padx=z(18), pady=z(9))''',
'''    l = RButton(_chk, text=text, command=None, bg=SURFACE, fg=MUTED, cursor="arrow",
                font=(UIF, zt(14, 9), "bold"), padx=z(18), pady=z(9), radius=zt(40, 20))''')

# port pickers: rounded button that opens the port list
R('port_menu.pack(fill="x")\n', '''def _post_menu(om, btn):
    m = om["menu"]
    m.tk_popup(btn.winfo_rootx(), btn.winfo_rooty() + btn.winfo_height())
port_btn = RButton(cframe, textvariable=port_var, suffix="  \\u25be", bg=SURFACE2, fg=TEXT,
                   font=F_SMALL, pady=zt(6, 8), command=lambda: _post_menu(port_menu, port_btn))
port_btn.pack(fill="x")
''')
R('su_port_menu.pack(side="left", fill="x", expand=True, padx=(z(0), z(8)))\n',
  '''su_port_btn = RButton(_c1r, textvariable=port_var, suffix="  \\u25be", bg=SURFACE2, fg=TEXT,
                      font=F_LABEL, pady=zt(10, 10),
                      command=lambda: _post_menu(su_port_menu, su_port_btn))
su_port_btn.pack(side="left", fill="x", expand=True, padx=(z(0), z(8)))
''')

# step badges on the Get-ready page
R('''    badge = tk.Label(hdr, text=str(num), bg=SURFACE2, fg=MUTED, width=2,
                     font=(UIF, zt(18, 11), "bold"), padx=z(4), pady=z(2))''',
  '''    badge = RButton(hdr, text=str(num), command=None, bg=SURFACE2, fg=MUTED, width=2,
                    cursor="arrow", font=(UIF, zt(18, 11), "bold"), padx=z(6), pady=z(4),
                    radius=zt(40, 20))''')

# floating Advanced panel
R('''adv_frame = tk.Frame(root, bg=SURFACE, highlightthickness=2,
                     highlightbackground=BORDER, highlightcolor=BORDER)   # [v2.9.4] floating panel''',
  '''adv_frame = RoundBox(root, fill=SURFACE, outline=BORDER)   # [v2.9.4] floating panel''')

# live canvases with rounded dark backgrounds
R('''    c = tk.Canvas(inner, width=w, height=h, bg="#0a0f1a", highlightthickness=0)''',
  '''    c = tk.Canvas(inner, width=w, height=h, bg=SURFACE, highlightthickness=0)''')
R('''    c.delete("all")
    # the table the arm support rests on at θ = 0''', '''    c.delete("all")
    _rr_bg(c, "#0a0f1a")
    # the table the arm support rests on at θ = 0''')
R('''    gauge = tk.Canvas(inner, width=z(170), height=z(260), bg="#0a0f1a", highlightthickness=0)''',
  '''    gauge = tk.Canvas(inner, width=z(170), height=z(260), bg=SURFACE, highlightthickness=0)''')
R('''    c.delete("all")
    W, H = z(170), z(260)''', '''    c.delete("all")
    _rr_bg(c, "#0a0f1a")
    W, H = z(170), z(260)''')
R('''gcanvas_o = tk.Frame(game_frame, bg=BORDER, padx=z(1), pady=z(1))''',
  '''gcanvas_o = tk.Frame(game_frame, bg=BG)''')
R('''canvas = tk.Canvas(gcanvas_o, width=SCREEN_W, height=SCREEN_H,
                   bg="#0a0f1a", highlightthickness=0)''', '''canvas = tk.Canvas(gcanvas_o, width=SCREEN_W, height=SCREEN_H,
                   bg=BG, highlightthickness=0)''')
R('''    canvas.create_rectangle(0, 0, SCREEN_W, SCREEN_H, fill="#0a0f1a", outline="")''',
  '''    _rr_bg(canvas, "#0a0f1a")''')

R('''    sb = tk.Scrollbar(wrap, orient="vertical", command=cv.yview, width=zt(16, 24),
                      bg=HANDLE, troughcolor=SURFACE2, activebackground=HANDLE,
                      bd=0, highlightthickness=0, elementborderwidth=0, relief="flat")''',
  '''    sb = None                                   # created below (needs the canvas)''')
R('''    cv.pack(side="left", fill="both", expand=True)
    body = tk.Frame(cv, bg=SURFACE)''', '''    cv.pack(side="left", fill="both", expand=True)
    sb = RScroll(wrap, cv)
    body = tk.Frame(cv, bg=SURFACE)''')
# slider rows a little tighter (the round slider is taller than the old trough)
R('''    holder.pack(fill="x", pady=(z(4), z(3)))''', '''    holder.pack(fill="x", pady=(z(2), z(1)))''')
open(dst_path, "w", encoding="utf-8").write(s)
print("pass 5 written")
