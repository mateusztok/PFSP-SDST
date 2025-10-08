# gui_live.py
# Uruchom: python3 gui_live.py
import io, csv, math, threading, subprocess, tkinter as tk
from tkinter import ttk, filedialog, messagebox
from collections import defaultdict, deque
import logging
import os

# Konfiguracja logowania
logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

logger.info("=== PFSP GUI STARTING ===")
logger.info(f"Working directory: {os.getcwd()}")
logger.info(f"Python executable: {subprocess.sys.executable}")

ALGO_SCHEMAS = {
    "Quick NEH": [],  # Podstawowy NEH bez parametrów
    "NEH with Progress": [],  # NEH z logowaniem progressu dla GUI
    "Simulated Annealing": [
        ("Temperatura początkowa", "float", {"min": 1e-9}, 100.0),
        ("Temperatura końcowa", "float", {"min": 1e-9}, 0.1),
        ("Współczynnik chłodzenia", "float", {"min": 1e-6, "max": 1.0}, 0.95),
        ("Iteracje na poziom", "int", {"min": 1}, 200),
    ],
    "Adaptive Simulated Annealing": [
        ("Temperatura początkowa", "float", {"min": 1e-9}, 100.0),
        ("Temperatura końcowa", "float", {"min": 1e-9}, 0.1),
        ("Współczynnik adaptacji", "float", {"min": 1e-6, "max": 1.0}, 0.98),
        ("Iteracje na poziom", "int", {"min": 1}, 150),
    ],
    "Quick NEH + SA": [],  # Kombinacja bez dodatkowych parametrów
    "NEH Progress + SA": [],  # Kombinacja z progressem
    "Quick NEH + Adaptive SA": [],  # Kombinacja z ASA
}
ERROR_BG = "#ffdddd"

class App(tk.Tk):
    def __init__(self):
        logger.info("Initializing GUI App...")
        super().__init__()
        self.title("PFSP – Live GUI (Canvas)")
        self.geometry("1100x820")
        self.minsize(900, 680)
        logger.info("GUI window created")

        self.file_var = tk.StringVar()
        self.method_var = tk.StringVar(value=list(ALGO_SCHEMAS.keys())[0])
        self.param_vars, self.param_widgets = {}, {}
        self.size_var = tk.StringVar(value="m=?, n=?")
        self.best_var = tk.StringVar(value="Najlepszy: —")
        self.m = self.n = None

        # Live process and buffers
        self.proc = None
        self.lock = threading.Lock()
        self.current_iter = 0
        self.slots_by_iter = defaultdict(list)   # iter -> list of slots dicts
        self.last_complete_iter = 0

        self._build_ui()
        self._render_params()
        logger.info("GUI initialization complete")

    def _build_ui(self):
        menubar = tk.Menu(self)
        fm = tk.Menu(menubar, tearoff=0)
        fm.add_command(label="Otwórz plik…", command=self._choose_file)
        fm.add_separator()
        fm.add_command(label="Wyjście", command=self.destroy)
        menubar.add_cascade(label="Plik", menu=fm)
        self.config(menu=menubar)

        top = ttk.Frame(self); top.pack(fill="x", padx=10, pady=6)
        lf_file = ttk.LabelFrame(top, text="Dane wejściowe")
        lf_file.pack(side="left", fill="x", expand=True, padx=(0,8))
        ttk.Entry(lf_file, textvariable=self.file_var).pack(side="left", fill="x", expand=True, padx=8, pady=8)
        ttk.Button(lf_file, text="Wybierz plik…", command=self._choose_file).pack(side="left", padx=8, pady=8)

        lf_size = ttk.LabelFrame(top, text="Rozmiar (1. wiersz)")
        lf_size.pack(side="left")
        ttk.Label(lf_size, textvariable=self.size_var).pack(padx=10, pady=8)

        middle = ttk.Frame(self); middle.pack(fill="x", padx=10, pady=6)
        lf_pick = ttk.LabelFrame(middle, text="Podejście")
        lf_pick.pack(side="left", fill="y", padx=(0,8))
        ttk.Label(lf_pick, text="Rodzaj algorytmu:").pack(padx=8, pady=(8,4))
        cb = ttk.Combobox(lf_pick, textvariable=self.method_var, values=list(ALGO_SCHEMAS.keys()),
                          state="readonly", width=28)
        cb.pack(padx=8, pady=(0,8))
        cb.bind("<<ComboboxSelected>>", lambda e: self._render_params())

        self.lf_params = ttk.LabelFrame(middle, text="Parametry")
        self.lf_params.pack(side="left", fill="x", expand=True)

        live = ttk.LabelFrame(self, text="Status na żywo")
        live.pack(fill="x", padx=10, pady=6)
        ttk.Label(live, textvariable=self.best_var, font=("Segoe UI", 11, "bold")).pack(side="left", padx=12, pady=8)

        ctrl = ttk.Frame(self); ctrl.pack(fill="x", padx=10, pady=6)
        ttk.Button(ctrl, text="Start LIVE", command=self._start_live).pack(side="right", padx=4)
        ttk.Button(ctrl, text="Stop", command=self._stop_proc).pack(side="right", padx=4)

        gantt_frame = ttk.LabelFrame(self, text="Harmonogram (Canvas)")
        gantt_frame.pack(fill="both", expand=True, padx=10, pady=6)
        self.xscroll = tk.Scrollbar(gantt_frame, orient="horizontal")
        self.xscroll.pack(side="bottom", fill="x")
        self.gantt = tk.Canvas(gantt_frame, bg="white", xscrollcommand=self.xscroll.set, height=420)
        self.gantt.pack(fill="both", expand=True, padx=6, pady=6)
        self.xscroll.config(command=self.gantt.xview)

        logf = ttk.LabelFrame(self, text="Log")
        logf.pack(fill="both", expand=True, padx=10, pady=(0,10))
        self.txt = tk.Text(logf, height=10)
        self.txt.pack(fill="both", expand=True, padx=6, pady=6)

    def _choose_file(self):
        logger.info("File dialog opened")
        path = filedialog.askopenfilename(filetypes=[("TXT/CSV", "*.txt *.csv"), ("Wszystkie", "*.*")])
        if not path: 
            logger.info("No file selected")
            return
        logger.info(f"File selected: {path}")
        self.file_var.set(path)
        self._read_size(path)

    def _read_size(self, path):
        logger.info(f"Reading file size from: {path}")
        self.m=self.n=None
        try:
            with open(path,"r",encoding="utf-8") as f:
                content = f.read()
            logger.debug(f"File content preview: {content[:100]}...")
            
            # Sprawdź format "jobs = X" i "machines = Y"
            import re
            jobs_match = re.search(r'jobs\s*=\s*(\d+)', content)
            machines_match = re.search(r'machines\s*=\s*(\d+)', content)
            
            if jobs_match and machines_match:
                n = int(jobs_match.group(1))   # jobs = n (zadania)
                m = int(machines_match.group(1))  # machines = m (maszyny)
                logger.info(f"Found format: jobs={n}, machines={m}")
                if m>0 and n>0:
                    self.m,self.n=m,n
                    self.size_var.set(f"m={m}, n={n}")
                    logger.info(f"Successfully parsed: m={m}, n={n}")
                else:
                    logger.error("Invalid m,n values")
                    self.size_var.set("Błędny format: m,n>0")
            else:
                logger.info("Format 'jobs = X' not found, trying fallback format")
                # Fallback: spróbuj stary format "m n"
                first_line = content.split('\n')[0].strip()
                t=first_line.replace(","," ").split()
                if len(t)>=2:
                    m=int(t[0]); n=int(t[1])
                    if m>0 and n>0:
                        self.m,self.n=m,n
                        self.size_var.set(f"m={m}, n={n}")
                        logger.info(f"Fallback format parsed: m={m}, n={n}")
                    else:
                        logger.error("Invalid fallback m,n values")
                        self.size_var.set("Błędny format: m,n>0")
                else:
                    logger.error("No valid format found")
                    self.size_var.set("Błędny format: 'm n …' lub 'jobs = n'")
        except Exception as e:
            logger.error(f"Error reading file: {e}")
            self.size_var.set(f"Błąd: {e}")

    def _clear_params(self):
        for w in self.lf_params.winfo_children():
            w.destroy()
        self.param_vars.clear(); self.param_widgets.clear()

    def _render_params(self):
        self._clear_params()
        schema = ALGO_SCHEMAS[self.method_var.get()]
        for r,(label,kind,rules,default) in enumerate(schema):
            ttk.Label(self.lf_params,text=label+":").grid(row=r,column=0,padx=8,pady=4,sticky="w")
            if kind=="combo":
                var=tk.StringVar(value=default)
                w=ttk.Combobox(self.lf_params,textvariable=var,values=rules.get("choices",[]),state="readonly",width=16)
            elif kind in ("int","float"):
                var=tk.StringVar(value=str(default)); w=ttk.Entry(self.lf_params,textvariable=var,width=14)
            else:
                var=tk.StringVar(value=str(default)); w=ttk.Entry(self.lf_params,textvariable=var,width=16)
            w.grid(row=r,column=1,padx=8,pady=4,sticky="w")
            self.param_vars[label]=(var,kind,rules); self.param_widgets[label]=w

    # --------- LIVE subprocess ---------
    def _start_live(self):
        logger.info("=== START LIVE clicked ===")
        if not self.file_var.get():
            logger.warning("No file selected")
            messagebox.showwarning("Brak pliku","Wybierz plik z danymi."); return
        
        logger.info(f"Selected file: {self.file_var.get()}")
        logger.info(f"Problem size: m={self.m}, n={self.n}")
        
        self.best_var.set("Najlepszy: —")
        # Wyczyść bufor slotów i Canvas
        with self.lock:
            self.slots_by_iter.clear()
            self.current_iter = 0
            self.last_complete_iter = 0
        self.gantt.delete("all")

        # Uruchom C++ solver — ścieżka dla WSL/Linux
        binary_path = "../../low_layer/bin/pfsp_sdst"
        
        # Mapowanie nazw GUI na nazwy algorytmów C++
        algorithm_map = {
            "Quick NEH": "quick_neh",
            "NEH with Progress": "neh_progress", 
            "Simulated Annealing": "simulated_annealing",
            "Adaptive Simulated Annealing": "adaptive_sa",
            "Quick NEH + SA": "quick_neh+sa",
            "NEH Progress + SA": "neh_progress+sa", 
            "Quick NEH + Adaptive SA": "quick_neh+adaptive_sa"
        }
        
        selected_algo = self.method_var.get()
        cpp_algorithm = algorithm_map.get(selected_algo, "neh_progress")  # fallback
        
        logger.info(f"Selected GUI algorithm: {selected_algo}")
        logger.info(f"Mapped to C++ algorithm: {cpp_algorithm}")
        
        cmd = [binary_path, self.file_var.get(), cpp_algorithm]
        
        logger.info(f"Command to execute: {' '.join(cmd)}")
        logger.info(f"Checking if binary exists: {binary_path}")
        
        import os
        if os.path.exists(binary_path):
            logger.info("Binary found!")
        else:
            logger.error(f"Binary NOT found at {binary_path}")
            logger.info(f"Current working directory: {os.getcwd()}")
            logger.info(f"Files in current dir: {os.listdir('.')}")
            
        self.txt.insert("end", f"$ {' '.join(cmd)}\n"); self.txt.see("end")
        t = threading.Thread(target=self._stream_worker, args=(cmd,), daemon=True)
        t.start()
        logger.info("Thread started")

    def _stop_proc(self):
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            self.txt.insert("end","[i] Zatrzymano proces.\n"); self.txt.see("end")

    def _stream_worker(self, cmd):
        logger.info(f"Stream worker started with command: {cmd}")
        try:
            # Uwaga: bufsize=1 + text=True dla buforowania liniowego [platformowo]
            logger.info("Creating subprocess...")
            self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                         text=True, bufsize=1, universal_newlines=True)
            logger.info(f"Process created with PID: {self.proc.pid}")
            
            with self.proc.stdout:
                line_count = 0
                for line in iter(self.proc.stdout.readline, ''):
                    line_count += 1
                    s = line.rstrip("\n")
                    if line_count <= 10:  # Log first 10 lines
                        logger.debug(f"Output line {line_count}: {s}")
                    
                    self.txt.after(0, lambda L=line: (self.txt.insert("end", L), self.txt.see("end")))
                    if not s:
                        continue
                    # Parsowanie prostych prefiksów API
                    if s.startswith("NEH_PROGRESS;"):
                        logger.debug(f"Progress: {s}")
                        self._handle_progress(s)
                    elif s.startswith("NEH_CHOICE;"):
                        pass  # opcjonalnie: można pokazywać job/best_pos
                    elif s.startswith("SLOT;"):
                        logger.debug(f"Slot: {s}")
                        self._handle_slot(s)
                    elif s.startswith("FRAME_END;"):
                        logger.debug(f"Frame end: {s}")
                        self._handle_frame_end(s)
                    elif s.startswith("NEH_RESULT;") or s.startswith("SA_RESULT;") or s.startswith("ASA_RESULT;") or s.startswith("QUICK_NEH_RESULT;"):
                        logger.info(f"Result: {s}")
                        self._handle_result(s)
                    elif s == "END":
                        logger.info("Process ended normally")
                        break
                    elif s.startswith("ERROR;"):
                        logger.error(f"C++ error: {s}")
                        # błąd z C++ — pokaż userowi
                        self.txt.after(0, lambda L=line: (self.txt.insert("end", L), self.txt.see("end")))
                    # inne linie ignorujemy lub wypisujemy w logu
            
            self.proc.wait()
            logger.info(f"Process finished with return code: {self.proc.returncode}")
            
        except Exception as e:
            logger.error(f"Error in stream worker: {e}")
            import traceback
            logger.error(f"Traceback: {traceback.format_exc()}")

    # --------- Parsowanie linii API ---------
    @staticmethod
    def _kv_parse(fields):
        out={}
        for f in fields:
            if "=" in f:
                k,v = f.split("=",1)
                out[k]=v
        return out

    def _handle_progress(self, s: str):
        # NEH_PROGRESS;iter=K;percent=P;cmax=C;delta=D;seq=a,b,c
        try:
            _, *rest = s.split(";")
            kv = self._kv_parse(rest)
            it = int(kv.get("iter","0"))
            cmax = int(float(kv.get("cmax","0")))
            seq = [int(x) for x in kv.get("seq","").split(",") if x!=""]
            self.current_iter = max(self.current_iter, it)
            self.best_var.set(f"Iter {it}: Cmax={cmax} | perm={seq}")
        except Exception:
            pass

    def _handle_slot(self, s: str):
        # SLOT;iter=K;machine=M;prev=I;job=J;setup=S;start=T0;end=T1
        try:
            _, *rest = s.split(";")
            kv = self._kv_parse(rest)
            it = int(kv.get("iter","0"))
            slot = {
                'iter': it,
                'machine': int(kv.get("machine","0")),
                'prev': int(kv.get("prev","-1")),
                'job': int(kv.get("job","0")),
                'setup': int(kv.get("setup","0")),
                'start': int(float(kv.get("start","0"))),
                'end': int(float(kv.get("end","0"))),
            }
            with self.lock:
                self.slots_by_iter[it].append(slot)
        except Exception:
            pass

    def _handle_frame_end(self, s: str):
        # FRAME_END;iter=K  => przerysuj Canvas z danymi dla tej iteracji
        try:
            _, *rest = s.split(";")
            kv = self._kv_parse(rest)
            it = int(kv.get("iter","0"))
            with self.lock:
                segs = list(self.slots_by_iter.get(it, []))
                self.last_complete_iter = max(self.last_complete_iter, it)
            if segs:
                self.gantt.after(0, lambda segs=segs: self.draw_gantt(segs))
        except Exception:
            pass

    def _handle_result(self, s: str):
        # NEH_RESULT;perm=...;cmax=...
        try:
            _, *rest = s.split(";")
            kv = self._kv_parse(rest)
            cmax = int(float(kv.get("cmax","0")))
            perm = [int(x) for x in kv.get("perm","").split(",") if x!=""]
            self.best_var.set(f"Najlepszy: Cmax={cmax} | perm={perm}")
        except Exception:
            pass

    # --------- Canvas / Gantt w czasie rzeczywistym ---------
    def draw_gantt(self, slots, scale=5, row_h=28, pad_left=120, pad_top=16, show_setups=True):
        c = self.gantt
        c.delete("all")
        if not slots: return

        machines = sorted({t['machine'] for t in slots})
        max_end = 0
        for t in slots:
            max_end = max(max_end, t['end'])
            if show_setups and t['setup']>0:
                max_end = max(max_end, t['end'])  # setup nie zwiększa końca, ale start-setup nie wpływa na oś max
        width = pad_left + int(max_end * scale) + 80
        height = pad_top + len(machines) * row_h + 30
        c.config(scrollregion=(0,0,width,height))
        self.update_idletasks()
        # nie przewijamy do 0, aby nie skakało — zostawiamy bieżące położenie

        # siatka i etykiety maszyn
        for idx, m in enumerate(machines):
            y = pad_top + idx * row_h
            if idx % 2 == 1:
                c.create_rectangle(pad_left, y-2, width, y+row_h-6, fill="#fafafa", outline="")
            c.create_text(10, y + row_h/2, text=f"M{m}", anchor="w")
            c.create_line(pad_left, y + row_h-4, width, y + row_h-4, fill="#e6e6e6")

        palette=["#66c2a5","#fc8d62","#8da0cb","#e78ac3","#a6d854","#ffd92f","#e5c494","#b3b3b3"]
        # Pokoloruj operacje i przezbrojenia
        for t in slots:
            y = pad_top + machines.index(t['machine']) * row_h
            x0 = pad_left + int(t['start'] * scale)
            x1 = pad_left + int(t['end'] * scale)
            color = palette[t['job'] % len(palette)]
            # pasek operacji
            c.create_rectangle(x0, y, x1, y + row_h - 8, fill=color, outline="#444")
            c.create_text((x0+x1)//2, y + (row_h-8)//2, text=f"J{t['job']}", fill="white")
            # pasek przezbrojenia (jeśli >0)
            if show_setups and t['setup']>0:
                xs0 = pad_left + int((t['start'] - t['setup']) * scale)
                xs1 = pad_left + int(t['start'] * scale)
                c.create_rectangle(xs0, y+2, xs1, y + row_h - 10, fill="#ff9999", outline="#aa4444")
                # mała etykieta s
                c.create_text(xs0+4, y + 6, text=f"s={t['setup']}", anchor="w", fill="#7a1f1f", font=("Segoe UI",8))

        # oś czasu
        step = max(1, max_end // 10)
        for t in range(0, max_end + 1, step):
            x = pad_left + int(t * scale)
            c.create_line(x, pad_top - 6, x, pad_top + len(machines) * row_h, fill="#d0d0d0")
            c.create_text(x, pad_top - 8, text=str(t), anchor="s")

if __name__=="__main__":
    logger.info("Starting PFSP GUI application...")
    try:
        app = App()
        logger.info("App created successfully, starting mainloop...")
        app.mainloop()
        logger.info("Application closed")
    except Exception as e:
        logger.error(f"Fatal error: {e}")
        import traceback
        logger.error(f"Traceback: {traceback.format_exc()}")
