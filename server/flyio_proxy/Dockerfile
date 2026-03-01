FROM python:3.11-slim

ENV PIP_DISABLE_PIP_VERSION_CHECK=1 \
    PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY app.py .

# Fly će proslijediti promet na ovaj port
ENV PORT=8080
EXPOSE 8080

# Production server
CMD ["gunicorn", "-b", "0.0.0.0:8080", "app:app"]
