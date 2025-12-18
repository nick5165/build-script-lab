from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
import redis
import json
import os

app = FastAPI()

class TextRequest(BaseModel):
    text: str
    
REDIS_HOST = os.getenv("REDIS_HOST", 'redis')
QUEUE_NAME = 'ml_task'

redis_client = redis.Redis(host=REDIS_HOST, port=6379, db=0)

@app.post("/analyze")
async def analyze_text(request: TextRequest):
    try:
        
        task = {
            'text': request.text,
            'status': 'new'
        }
        
        message = json.dumps(task)
        redis_client.lpush(QUEUE_NAME, message)
        
        return {"status": "Text pushed to Redis", "preview": request.text[:30]}
    
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))