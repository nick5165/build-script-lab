import redis
import json
import time
import os
import random
from textblob import TextBlob

REDIS_HOST = os.getenv('REDIS_HOST', 'redis')
QUEUE_NAME = 'ml_task'
WORKER_ID = os.getenv('HOSTNAME', 'worker')

print(f"[{WORKER_ID}] Connecting to Redis...")
r = redis.Redis(host=REDIS_HOST, port=6379)

def ml_task(text):
    blob = TextBlob(text)
    sentiment = blob.sentiment.polarity
    processing_time = random.uniform(1.0, 4.0)
    time.sleep(processing_time)
    label = "POSITIVE" if sentiment > 0 else "NEGATIVE" if sentiment < 0 else "NEUTRAL"
    return label, processing_time

print(f"[{WORKER_ID}] Ready to process tasks from queue: {QUEUE_NAME}")

while True:
    try:
        queue, data = r.brpop(QUEUE_NAME, timeout=0)
        
        if data:
            task = json.loads(data)
            text = task.get('text', '')
            print(f"[{WORKER_ID}] Took task: {text[:20]}...")
            
            result, duration = ml_task(text)
            
            print(f"[{WORKER_ID}] DONE in {duration:.2f}s | Result: {result}")
            
    except redis.ConnectionError:
        print(f"[{WORKER_ID}] Redis connection lost. Retrying in 5s...")
        time.sleep(5)
    except Exception as e:
        print(f"[{WORKER_ID}] Error: {e}")