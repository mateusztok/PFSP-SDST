# validator.py
import os

def validate_file_content(path):
    """Rygorystyczne sprawdzanie poprawności formatu pliku."""
    if not os.path.isfile(path): return False, "Błąd ścieżki.", 0, 0
    try:
        n, m = 0, 0
        has_proc_time = False
        has_setup_time = False
        machine_headers = set()

        with open(path, 'r') as f:
            lines = f.readlines()

        for line in lines:
            clean_line = line.strip()
            if not clean_line: continue

            if clean_line.startswith('jobs'):
                if '=' not in clean_line: return False, "Błąd: Brak '=' w 'jobs'.", 0, 0
                n = int(clean_line.split('=')[1].strip())
            elif clean_line.startswith('machines'):
                if '=' not in clean_line: return False, "Błąd: Brak '=' w 'machines'.", 0, 0
                m = int(clean_line.split('=')[1].strip())
            elif clean_line.startswith('proc_time'):
                has_proc_time = '=' in clean_line
            elif clean_line.startswith('setup_time'):
                has_setup_time = '=' in clean_line
            elif clean_line.startswith('# machine'):
                try:
                    m_idx = int(clean_line.replace('# machine', '').strip())
                    machine_headers.add(m_idx)
                except: pass

        if n <= 0 or m <= 0: return False, "Błąd: jobs/machines muszą być > 0.", 0, 0
        if not has_proc_time: return False, "Błąd: Brak sekcji 'proc_time ='.", 0, 0
        if not has_setup_time: return False, "Błąd: Brak sekcji 'setup_time ='.", 0, 0
        
        for i in range(1, m + 1):
            if i not in machine_headers:
                return False, f"Błąd: Brak macierzy dla maszyny {i}.", 0, 0

        return True, "", n, m
    except Exception as e:
        return False, f"Błąd odczytu: {str(e)}", 0, 0