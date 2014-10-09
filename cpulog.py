import psutil
import sys
import subprocess
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

def main():
    cmd = sys.argv[1:]
    process = subprocess.Popen(cmd)

    cpu_usages = []

    while process.poll() == None:
        cpu_usage = psutil.cpu_percent(interval=0.5, percpu=True)
        filtered_usage = filter(lambda (i, x): x > 1, enumerate(cpu_usage))
        print filtered_usage
        cpu_usages.append(cpu_usage)

    plt.plot(cpu_usages)
    plt.gca().spines['right'].set_color('none')
    plt.gca().spines['top'].set_color('none')
    plt.savefig("test.pdf", format='pdf')


if __name__ == "__main__":
    main()
