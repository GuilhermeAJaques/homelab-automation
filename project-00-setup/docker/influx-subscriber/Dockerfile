FROM python:3.11-slim
WORKDIR /app
COPY requirements.txt .
RUN pip install -r requirements.txt
COPY subscriber.py .
CMD ["python", "subscriber.py"]