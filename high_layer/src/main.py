import os
import threading
import subprocess
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from collections import defaultdict

# Definicja parametrów dla GUI, które mogą być przekazywane do C++
ALGO_SCHEMAS = {
    "Heurystyka (NEH)": [
        ("Typ", "fixed", {}, "neh"),
    ],
    "Metaheurystyka (Simulated Annealing)": [
        ("Typ", "fixed", {}, "simulated_annealing"),
        ("Iteracje", "int", {"min": 1}, 50000),
        ("T Start", "float", {"min": 0.1}, 100.0),
        ("Chłodzenie", "float", {"min": 0.001, "max": 0.999}, 0.9975),
    ],
    "NEH + Simulated Annealing": [
        ("Typ", "fixed", {}, "neh"),
        ("Dodatek", "fixed", {}, "simulated_annealing"),
    ]
}

class PFSPGui(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("PFSP-SDST Solver – Profesjonalna Wizualizacja")
        self.geometry("1200x900")
        
        # Ścieżka do binarki C++
        self.executable_path = os.path.join(os.path.dirname(__file__), "../../low_layer/bin/pfsp_sdst")
        
        self.file_var = tk.StringVar()
        self.method_var = tk.StringVar(value=list(ALGO_SCHEMAS.keys())[0])
        self.status_var = tk.StringVar(value="Gotowy")
        self.cmax_var = tk.StringVar(value="Cmax: —")
        
        self.proc = None
        self.lock = threading.Lock()
        self.slots_by_iter = defaultdict(list)
        
        self._setup_ui()

    def _setup_ui(self):
        # Panel górny - Konfiguracja
        top = ttk.LabelFrame(self, text="Konfiguracja")
        top.pack(fill="x", padx=10, pady=5)
        
        ttk.Entry(top, textvariable=self.file_var).pack(side="left", fill="x", expand=True, padx=5, pady=5)
        ttk.Button(top, text="Wybierz plik", command=self._open_file).pack(side="left", padx=5)
        ttk.Combobox(top, textvariable=self.method_var, values=list(ALGO_SCHEMAS.keys()), state="readonly", width=30).pack(side="left", padx=5)

        # Status Bar
        status_frame = ttk.Frame(self)
        status_frame.pack(fill="x", padx=10)
        ttk.Label(status_frame, textvariable=self.status_var, font=("Segoe UI", 10, "italic")).pack(side="left")
        ttk.Label(status_frame, textvariable=self.cmax_var, font=("Segoe UI", 12, "bold"), foreground="#2980b9").pack(side="right")

        # Kontener wykresu z paskami przewijania
        self.canvas_container = ttk.LabelFrame(self, text="Wykres Gantta (Live)")
        self.canvas_container.pack(fill="both", expand=True, padx=10, pady=5)

        # Paski przewijania
        self.x_scroll = ttk.Scrollbar(self.canvas_container, orient="horizontal")
        self.y_scroll = ttk.Scrollbar(self.canvas_container, orient="vertical")
        
        self.canvas = tk.Canvas(
            self.canvas_container, 
            bg="#ffffff", 
            xscrollcommand=self.x_scroll.set, 
            yscrollcommand=self.y_scroll.set,
            highlightthickness=0
        )
        
        self.x_scroll.config(command=self.canvas.xview)
        self.y_scroll.config(command=self.canvas.yview)
        
        self.x_scroll.pack(side="bottom", fill="x")
        self.y_scroll.pack(side="right", fill="y")
        self.canvas.pack(side="left", fill="both", expand=True)

        # Logi
        self.log_text = tk.Text(self, height=8, font=("Consolas", 9), bg="#f8f9fa", relief="flat")
        self.log_text.pack(fill="x", padx=10, pady=5)

        # Przyciski sterujące
        btns = ttk.Frame(self)
        btns.pack(fill="x", padx=10, pady=10)
        ttk.Button(btns, text="URUCHOM SOLVER", command=self._run_solver, style="Accent.TButton").pack(side="right", padx=5)
        ttk.Button(btns, text="ZATRZYMAJ", command=self._stop_solver).pack(side="right", padx=5)

    def _open_file(self):
        path = filedialog.askopenfilename(initialdir="../../data")
        if path:
            self.file_var.set(path)

    def _run_solver(self):
        if not self.file_var.get():
            messagebox.showwarning("Błąd", "Wybierz plik danych.")
            return
        if not os.path.exists(self.executable_path):
            messagebox.showerror("Błąd", "Nie znaleziono binarki C++. Zbuduj projekt w low_layer.")
            return

        schema = ALGO_SCHEMAS[self.method_var.get()]
        cmd = [self.executable_path, self.file_var.get()]
        for _, _, _, val in schema:
            cmd.append(str(val))

        self.log_text.delete("1.0", "end")
        self.canvas.delete("all")
        with self.lock:
            self.slots_by_iter.clear()
        self.status_var.set("Trwają obliczenia...")
        
        threading.Thread(target=self._read_output, args=(cmd,), daemon=True).start()

    def _read_output(self, cmd):
        try:
            self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
            for line in iter(self.proc.stdout.readline, ''):
                self.log_text.after(0, lambda l=line: (self.log_text.insert("end", l), self.log_text.see("end")))
                
                if line.startswith("SLOT;"):
                    self._parse_slot(line)
                elif line.startswith("FRAME_END;"):
                    self._render_gantt()
                elif "makespan:" in line.lower():
                    val = line.split(":")[-1].strip()
                    self.cmax_var.set(f"Cmax: {val}")
                    
            self.proc.wait()
            self.status_var.set("Zakończono")
        except Exception as e:
            messagebox.showerror("Błąd", str(e))

    def _parse_slot(self, line):
        try:
            parts = {p.split('=')[0]: p.split('=')[1] for p in line.strip().split(';') if '=' in p}
            it = int(parts.get('iter', 0))
            slot = {
                'm': int(parts.get('machine', 0)),
                'j': int(parts.get('job', 0)),
                's': float(parts.get('start', 0)),
                'e': float(parts.get('end', 0)),
                'setup': float(parts.get('setup', 0))
            }
            with self.lock:
                self.slots_by_iter[it].append(slot)
        except: pass

    def _render_gantt(self):
        with self.lock:
            if not self.slots_by_iter: return
            last_it = max(self.slots_by_iter.keys())
            slots = list(self.slots_by_iter[last_it])
        self.canvas.after(0, lambda: self._draw_slots(slots))

    def _draw_slots(self, slots):
        self.canvas.delete("all")
        if not slots: return
        
        # Konfiguracja wizualna
        px_per_unit = 5.0 # Skala czasu (można regulować)
        row_h = 40        # Wysokość wiersza maszyny
        margin_left = 70
        margin_top = 40
        padding_v = 10
        
        max_time = max(s['e'] for s in slots)
        machines = sorted(list(set(s['m'] for s in slots)))
        
        # Kolory (Material Design)
        colors = ["#1abc9c", "#3498db", "#9b59b6", "#f1c40f", "#e67e22", "#e74c3c", "#34495e", "#2ecc71"]
        
        # Rysowanie osi czasu i siatki
        grid_step = 10 if max_time < 200 else 50
        for t in range(0, int(max_time) + grid_step, grid_step):
            x = margin_left + t * px_per_unit
            self.canvas.create_line(x, margin_top-5, x, margin_top + len(machines)*(row_h+padding_v), fill="#ecf0f1")
            self.canvas.create_text(x, margin_top-15, text=str(t), font=("Segoe UI", 8), fill="#7f8c8d")

        # Rysowanie maszyn i operacji
        for i, m_id in enumerate(machines):
            y_base = margin_top + i * (row_h + padding_v)
            
            # Etykieta maszyny
            self.canvas.create_rectangle(5, y_base, margin_left-10, y_base+row_h, fill="#ecf0f1", outline="")
            self.canvas.create_text(margin_left//2-5, y_base + row_h//2, text=f"Maszyna {m_id}", font=("Segoe UI", 9, "bold"))

            # Zadania na danej maszynie
            m_slots = [s for s in slots if s['m'] == m_id]
            for s in m_slots:
                x_start = margin_left + s['s'] * px_per_unit
                x_end = margin_left + s['e'] * px_per_unit
                
                # Setup (Przezbrojenie) - kreskowany czerwony blok
                if s['setup'] > 0:
                    x_setup = x_start - (s['setup'] * px_per_unit)
                    self.canvas.create_rectangle(x_setup, y_base+row_h//2-5, x_start, y_base+row_h//2+5, 
                                               fill="#ff7675", outline="#d63031", dash=(4,2))
                
                # Operacja - główny blok
                job_color = colors[s['j'] % len(colors)]
                rect_id = self.canvas.create_rectangle(x_start, y_base, x_end, y_base + row_h, 
                                                     fill=job_color, outline="#2c3e50", width=1)
                
                # Numer zadania
                if (x_end - x_start) > 20: # Pisz tekst tylko jeśli blok jest wystarczająco szeroki
                    self.canvas.create_text((x_start+x_end)/2, y_base+row_h/2, 
                                          text=f"J{s['j']}", fill="white", font=("Segoe UI", 9, "bold"))

        # Ustawienie obszaru przewijania
        total_width = margin_left + max_time * px_per_unit + 100
        total_height = margin_top + len(machines) * (row_h + padding_v) + 50
        self.canvas.config(scrollregion=(0, 0, total_width, total_height))

    def _stop_solver(self):
        if self.proc:
            self.proc.terminate()
            self.status_var.set("Zatrzymano na żądanie")

if __name__ == "__main__":
    app = PFSPGui()
    app.mainloop()