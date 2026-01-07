# app_gui.py
import os
import threading
import subprocess
import tkinter as tk
from tkinter import filedialog, messagebox
from collections import defaultdict
import customtkinter as ctk

from config import *
from gantt_chart import ScrollableGanttFrame
from validator import validate_file_content

class PFSPGui(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title("PFSP-SDST Optimizer")
        self.geometry("1400x950")
        ctk.set_appearance_mode("dark")
        
        self.executable_path = os.path.join(os.path.dirname(__file__), "../../low_layer/bin/pfsp_sdst")
        self.file_var = tk.StringVar()
        self.method_var = tk.StringVar(value=list(ALGO_SCHEMAS.keys())[0])
        self.status_var = tk.StringVar(value="Gotowy")
        self.cmax_var = tk.StringVar(value="Cmax: —")
        self.seq_var = tk.StringVar(value="Kolejność: —")
        self.param_vars = {}
        self.proc = None
        self.lock = threading.Lock()
        self.slots_by_iter = defaultdict(list)
        
        self._setup_ui()
        self._update_params_ui()

    def _setup_ui(self):
        self.grid_columnconfigure(0, weight=1)
        self.grid_rowconfigure(2, weight=1)

        # Top Panel
        top = ctk.CTkFrame(self, fg_color=BG_COLOR)
        top.grid(row=0, column=0, sticky="ew", padx=10, pady=10)
        ctk.CTkEntry(top, textvariable=self.file_var, width=350).pack(side="left", padx=10)
        ctk.CTkButton(top, text="Otwórz plik", command=self._open_file).pack(side="left", padx=5)
        ctk.CTkOptionMenu(top, variable=self.method_var, values=list(ALGO_SCHEMAS.keys()), 
                          command=lambda _: self._update_params_ui(), width=250).pack(side="left", padx=10)

        self.params_container = ctk.CTkFrame(self, fg_color=BG_COLOR)
        self.params_container.grid(row=1, column=0, sticky="ew", padx=10, pady=5)
        
        self.gantt_container = ScrollableGanttFrame(self)
        self.gantt_container.grid(row=2, column=0, sticky="nsew", padx=10, pady=5)

        # Logs and Status
        bottom = ctk.CTkFrame(self, fg_color=BG_COLOR)
        bottom.grid(row=3, column=0, sticky="ew", padx=10, pady=5)
        self.log_text = ctk.CTkTextbox(bottom, height=130, fg_color=PLOT_AREA_BG, font=("Consolas", 11))
        self.log_text.pack(fill="x", padx=10, pady=5)

        footer = ctk.CTkFrame(bottom, fg_color="transparent")
        footer.pack(fill="x", padx=10, pady=5)
        ctk.CTkLabel(footer, textvariable=self.status_var).pack(side="left")
        
        res_frame = ctk.CTkFrame(footer, fg_color="transparent")
        res_frame.pack(side="right")
        ctk.CTkLabel(res_frame, textvariable=self.seq_var, font=("Segoe UI", 12), text_color=AXIS_COLOR).pack(side="left", padx=20)
        ctk.CTkLabel(res_frame, textvariable=self.cmax_var, font=("Segoe UI", 16, "bold"), text_color="#58a6ff").pack(side="left")
        
        btns = ctk.CTkFrame(self, fg_color="transparent")
        btns.grid(row=4, column=0, sticky="ew", padx=10, pady=10)
        ctk.CTkButton(btns, text="ZATRZYMAJ", command=self._stop_solver, fg_color="#da3633").pack(side="left", padx=5)
        ctk.CTkButton(btns, text="URUCHOM SOLVER", command=self._run_solver, fg_color="#238636").pack(side="right", padx=5)

    def _update_params_ui(self):
        for w in self.params_container.winfo_children(): w.destroy()
        self.param_vars = {}
        schema = ALGO_SCHEMAS[self.method_var.get()]
        ctk.CTkLabel(self.params_container, text="Parametry:", font=("Arial", 11, "bold")).pack(side="left", padx=15)
        for param in schema:
            label, p_type, default = param[0], param[1], param[2]
            if p_type == "fixed":
                self.param_vars[label] = tk.StringVar(value=str(default)); continue
            f = ctk.CTkFrame(self.params_container, fg_color="transparent"); f.pack(side="left", padx=10)
            ctk.CTkLabel(f, text=f"{label}:").pack(side="left")
            v = tk.StringVar(value=str(default)); self.param_vars[label] = v
            ctk.CTkEntry(f, textvariable=v, width=70).pack(side="left", padx=5)

    def _open_file(self):
        p = filedialog.askopenfilename(initialdir="../../data")
        if p: self.file_var.set(p)

    def _run_solver(self):
        path = self.file_var.get()
        valid, err, n, m = validate_file_content(path)
        if not valid: messagebox.showerror("Błąd danych", err); return
        
        algo = self.method_var.get(); warning, reasons = False, []
        if "NEH" in algo and n > 800: warning = True; reasons.append(f"- Faza NEH: Duża liczba zadań ({n}).")
        if "Simulated Annealing" in algo:
            try:
                it = int(self.param_vars.get("Iteracje", self.param_vars.get("SA Iteracje", tk.StringVar(value="0"))).get())
                if (it * n * m) > 100_000_000 or n > 500: warning = True; reasons.append(f"- Faza SA: Wysoka złożoność.")
            except: pass
        if warning and not messagebox.askyesno("Ostrzeżenie", "\n".join(reasons) + "\n\nKontynuować?"): return

        cmd = [self.executable_path, path]
        for p in ALGO_SCHEMAS[algo]: cmd.append(self.param_vars[p[0]].get())
        
        self.log_text.delete("1.0", "end"); self.cmax_var.set("Cmax: —"); self.seq_var.set("Kolejność: —")
        with self.lock: self.slots_by_iter.clear()
        self.gantt_container._show_placeholder(); self.status_var.set("Trwają obliczenia...")
        threading.Thread(target=self._read_output, args=(cmd,), daemon=True).start()

    def _read_output(self, cmd):
        try:
            self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
            for line in iter(self.proc.stdout.readline, ''):
                self.after(0, lambda l=line: (self.log_text.insert("end", l), self.log_text.see("end")))
                if "final best sequence:" in line.lower():
                    s = line.split(":")[-1].strip(); self.after(0, lambda seq=s: self.seq_var.set(f"Kolejność: {seq}"))
                if line.startswith("SLOT;"): self._parse_slot(line)
                elif line.startswith("FRAME_END;"): self._render_gantt()
                elif "makespan:" in line.lower():
                    v = line.split(":")[-1].strip(); self.after(0, lambda val=v: self.cmax_var.set(f"Cmax: {val}"))
            self.proc.wait(); self.after(0, lambda: self.status_var.set("Zakończono"))
        except Exception as e: self.after(0, lambda: messagebox.showerror("Błąd", str(e)))

    def _parse_slot(self, line):
        try:
            p = {i.split('=')[0]: i.split('=')[1] for i in line.strip().split(';') if '=' in i}
            s = {'m': int(p['machine']), 'j': int(p['job']), 's': float(p['start']), 'e': float(p['end']), 'setup': float(p['setup'])}
            with self.lock: self.slots_by_iter[int(p.get('iter', 0))].append(s)
        except: pass

    def _render_gantt(self):
        with self.lock:
            if not self.slots_by_iter: return
            slots = list(self.slots_by_iter[max(self.slots_by_iter.keys())])
        self.after(0, lambda: self.gantt_container.update_plot(slots))

    def _stop_solver(self):
        if self.proc: self.proc.terminate(); self.status_var.set("Zatrzymano")