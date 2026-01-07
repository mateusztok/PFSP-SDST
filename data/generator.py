import random

def generate_exact_format(file_name, num_jobs=900, num_machines=2):
    with open(file_name, 'w') as f:
        # Nagłówek (używamy '=' i spacji)
        f.write(f"jobs = {num_jobs}\n")
        f.write(f"machines = {num_machines}\n\n")

        # Sekcja czasów operacji
        f.write("proc_time =\n")
        for _ in range(num_jobs):
            # Dwie wartości na wiersz dla 2 maszyn
            p1 = random.randint(5, 15)
            p2 = random.randint(5, 15)
            f.write(f"{p1}  {p2}\n")
        f.write("\n")

        # Sekcja czasów przezbrojeń
        f.write("setup_time =\n")
        for m in range(1, num_machines + 1):
            f.write(f"# machine {m}\n")
            for i in range(num_jobs):
                row = []
                for j in range(num_jobs):
                    if i == j:
                        row.append(0) # Przekątna zawsze 0
                    else:
                        row.append(random.randint(5, 10))
                # Zapisujemy wiersz macierzy oddzielony dwiema spacjami dla czytelności
                f.write("  ".join(map(str, row)) + "\n")
            f.write("\n")

if __name__ == "__main__":
    generate_exact_format("instancja_900.txt")
    print("Plik 'instancja_900.txt' został wygenerowany pomyślnie.")