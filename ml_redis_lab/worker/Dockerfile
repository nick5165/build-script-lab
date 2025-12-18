FROM python:3.9-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
RUN python -m textblob.download_corpora
COPY . .
# -u нужен, чтобы видеть логи в реальном времени
CMD ["python", "-u", "model_runner.py"]