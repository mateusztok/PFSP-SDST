# config.py
# --- KONFIGURACJA UI (GitHub Dark Theme) ---
BG_COLOR = "#161b22"
PLOT_AREA_BG = "#0d1117"
AXIS_COLOR = "#8b949e"
TEXT_COLOR = "#e6edf3"
GRID_COLOR = "#30363d"
STRIPE_COLOR = "#1c2128"
SETUP_COLOR = "#f85149"

ALGO_SCHEMAS = {
    "Heurystyka (NEH)": [("Typ", "fixed", "neh")],
    "Metaheurystyka (Simulated Annealing)": [
        ("Typ", "fixed", "simulated_annealing"),
        ("Iteracje", "int", 50000),
        ("T Start", "float", 100.0),
        ("Chłodzenie", "float", 0.9975),
    ],
    "NEH + Simulated Annealing": [
        ("Typ", "fixed", "neh"),
        ("Dodatek", "fixed", "simulated_annealing"),
        ("SA Iteracje", "int", 25000),
        ("SA T Start", "float", 80.0),
        ("SA Chłodzenie", "float", 0.995),
    ]
}