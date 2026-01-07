# gantt_chart.py
import tkinter as tk
import customtkinter as ctk
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
from config import *

class ScrollableGanttFrame(ctk.CTkFrame):
    def __init__(self, parent, **kwargs):
        super().__init__(parent, **kwargs)
        self.configure(fg_color=BG_COLOR)

        self.v_scroll = ctk.CTkScrollbar(self, orientation="vertical")
        self.v_scroll.pack(side="right", fill="y")
        self.h_scroll = ctk.CTkScrollbar(self, orientation="horizontal")
        self.h_scroll.pack(side="bottom", fill="x")

        self.canvas_view = tk.Canvas(self, bg=BG_COLOR, highlightthickness=0,
                                     yscrollcommand=self.v_scroll.set, xscrollcommand=self.h_scroll.set)
        self.canvas_view.pack(side="left", fill="both", expand=True)
        self.v_scroll.configure(command=self.canvas_view.yview)
        self.h_scroll.configure(command=self.canvas_view.xview)

        self.fig_canvas = None
        self._color_cache = {}
        self.bars = []
        self.tooltip = None

    def _get_contrast_color(self, bg_color):
        r, g, b, _ = mcolors.to_rgba(bg_color)
        lum = 0.299*r + 0.587*g + 0.114*b
        return 'black' if lum > 0.5 else 'white'

    def update_plot(self, slots):
        if not slots: return
        machines = sorted(list(set(s['m'] for s in slots)))
        max_time = max(s['e'] for s in slots)
        
        px_per_machine, px_per_time_unit = 60, 5
        req_h = max(450, len(machines) * px_per_machine)
        req_w = max(900, max_time * px_per_time_unit + 100)

        if self.fig_canvas:
            self.fig_canvas.get_tk_widget().destroy()
            plt.close('all')

        fig = Figure(figsize=(req_w/100, req_h/100), dpi=100, facecolor=BG_COLOR)
        fig.subplots_adjust(left=0.06, right=0.98, top=0.95, bottom=0.15)
        ax = fig.add_subplot(111); ax.set_facecolor(PLOT_AREA_BG)

        job_ids = sorted(list(set(s['j'] for s in slots)))
        cmap = plt.get_cmap('tab20')
        for idx, j_id in enumerate(job_ids):
            if j_id not in self._color_cache: self._color_cache[j_id] = cmap(idx % 20)

        self.bars = []
        for i, m_id in enumerate(machines):
            if i % 2 == 0: ax.axhspan(m_id - 0.45, m_id + 0.45, facecolor=STRIPE_COLOR, zorder=0)

        for s in slots:
            m, j, start, end, setup = s['m'], s['j'], s['s'], s['e'], s['setup']
            color = self._color_cache[j]
            duration = end - start
            bar = ax.barh(m, duration, left=start, height=0.6, color=color, edgecolor="#ffffff", linewidth=0.5, zorder=3)
            rect = bar[0]
            rect.set_gid(f"Zadanie: {j}\nMaszyna: {m}\nStart: {start}\nKoniec: {end}\nSetup: {setup}")
            self.bars.append(rect)
            if setup > 0:
                ax.barh(m, setup, left=start-setup, height=0.3, color=SETUP_COLOR, alpha=0.6, hatch='////', zorder=2)
            if duration > (max_time * 0.008):
                ax.text(start + duration/2, m, f"J{j}", ha='center', va='center', color=self._get_contrast_color(color), 
                        fontsize=8, weight='bold', zorder=4)

        ax.set_yticks(machines)
        ax.set_yticklabels([f"M{i}" for i in machines], color=TEXT_COLOR, weight='bold')
        ax.tick_params(axis='y', pad=2); ax.tick_params(axis='x', colors=AXIS_COLOR)
        ax.set_xlim(0, max_time * 1.02); ax.set_ylim(min(machines)-0.8, max(machines)+0.8)
        ax.grid(True, axis='x', color=GRID_COLOR, linestyle='--', alpha=0.3)
        for s in ax.spines.values(): s.set_visible(False)

        self.fig_canvas = FigureCanvasTkAgg(fig, master=self.canvas_view)
        self.canvas_view.create_window((0, 0), window=self.fig_canvas.get_tk_widget(), anchor="nw")
        self.canvas_view.config(scrollregion=(0, 0, req_w, req_h))
        self.fig_canvas.mpl_connect("motion_notify_event", self._on_hover)
        self.fig_canvas.draw()

    def _on_hover(self, event):
        if event.inaxes is None:
            self._hide_tooltip(); return
        for rect in self.bars:
            cont, _ = rect.contains(event)
            if cont: self._show_tooltip(rect.get_gid()); return
        self._hide_tooltip()

    def _show_tooltip(self, text):
        if self.tooltip: self.tooltip.destroy()
        x, y = self.winfo_pointerx() + 15, self.winfo_pointery() + 15
        self.tooltip = tk.Toplevel(self); self.tooltip.wm_overrideredirect(True)
        self.tooltip.wm_geometry(f"+{x}+{y}")
        tk.Label(self.tooltip, text=text, justify='left', bg="#30363d", fg="#e6edf3", padx=8, pady=8, font=("Consolas", 9)).pack()

    def _hide_tooltip(self):
        if self.tooltip: self.tooltip.destroy(); self.tooltip = None

    def _show_placeholder(self):
        if self.fig_canvas: self.fig_canvas.get_tk_widget().destroy(); plt.close('all')
        self.canvas_view.delete("all")