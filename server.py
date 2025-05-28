#!/usr/bin/env python3
"""
Flask server for Beta Evolve local AI integration using HuggingFace models.
Provides OpenAI-compatible API endpoints for local AI inference.

Usage: python server.py <huggingface-model-name> [--port PORT] [--temperature TEMP]

Example:
    python server.py microsoft/DialoGPT-medium
    python server.py google/flan-t5-large --port 8080 --temperature 0.7
"""

import sys
import argparse
import logging
from flask import Flask, request, jsonify, render_template
from transformers import (
    AutoTokenizer, 
    AutoModelForCausalLM, 
    AutoModelForSeq2SeqLM,
)
from transformers.pipelines import pipeline
import torch
from datetime import datetime
import uuid

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = Flask(__name__)

class LocalAIServer:
    def __init__(self, model_name, temperature=0.7, max_length=1024):
        """Initialize the local AI server with a HuggingFace model."""
        self.model_name = model_name
        self.temperature = temperature
        self.max_length = max_length
        self.tokenizer = None
        self.model = None
        self.pipeline = None
        
        logger.info(f"Initializing model: {model_name}")
        self._load_model()
        
    def _load_model(self):
        """Load the HuggingFace model and tokenizer."""
        try:
            # Try to load as a text generation model first
            self.tokenizer = AutoTokenizer.from_pretrained(self.model_name, trust_remote_code=True)
            
            # Add padding token if it doesn't exist
            if self.tokenizer.pad_token is None:
                self.tokenizer.pad_token = self.tokenizer.eos_token
            
            # Determine device
            if torch.cuda.is_available():
                device = "cuda"
            elif torch.mps.is_available():
                device = "mps"
            else:
                device = "cpu"
            logger.info(f"Using device: {device}")
            
            # Try loading as causal LM first, then seq2seq
            try:
                self.model = AutoModelForCausalLM.from_pretrained(
                    self.model_name, 
                    trust_remote_code=True,
                    torch_dtype=torch.float16 if device == "cuda" else torch.float32
                ).to(device)
                self.model_type = "causal"
                logger.info("Loaded as causal language model")
            except:
                try:
                    self.model = AutoModelForSeq2SeqLM.from_pretrained(
                        self.model_name,
                        trust_remote_code=True,
                        torch_dtype=torch.float16 if device == "cuda" else torch.float32
                    ).to(device)
                    self.model_type = "seq2seq"
                    logger.info("Loaded as sequence-to-sequence model")
                except Exception as e:
                    logger.error(f"Failed to load model as either causal or seq2seq: {e}")
                    raise
                    
            # Create pipeline for easier inference
            if self.model_type == "causal":
                self.pipeline = pipeline(
                    "text-generation", 
                    model=self.model, 
                    tokenizer=self.tokenizer,
                    device=0 if device == "cuda" else -1
                )
            else:
                self.pipeline = pipeline(
                    "text2text-generation",
                    model=self.model,
                    tokenizer=self.tokenizer,
                    device=0 if device == "cuda" else -1
                )
                
            logger.info(f"Model loaded successfully: {self.model_name}")
            
        except Exception as e:
            logger.error(f"Failed to load model {self.model_name}: {e}")
            raise
    
    def generate_response(self, prompt, temperature=None, max_tokens=None):
        """Generate a response using the loaded model."""
        try:
            # Check if pipeline is initialized
            if self.pipeline is None:
                logger.error("Pipeline is not initialized")
                return "Error: Model pipeline is not properly initialized"
                
            temp = temperature if temperature is not None else self.temperature
            max_len = max_tokens if max_tokens is not None else self.max_length
            
            if self.model_type == "causal":
                # For causal models, we generate continuation
                # Set pad_token_id only if tokenizer is available
                pipeline_kwargs = {
                    "max_new_tokens": max_len,
                    "temperature": temp,
                    "do_sample": True,
                    "num_return_sequences": 1,
                    "return_full_text": False
                }
                
                if self.tokenizer and hasattr(self.tokenizer, 'eos_token_id') and self.tokenizer.eos_token_id is not None:
                    pipeline_kwargs["pad_token_id"] = self.tokenizer.eos_token_id
                
                responses = self.pipeline(prompt, **pipeline_kwargs)
                # Handle different response types
                if isinstance(responses, list):
                    if responses and isinstance(responses[0], dict) and 'generated_text' in responses[0]:
                        response_text = responses[0]['generated_text'].strip()
                    else:
                        response_text = str(responses[0]).strip()
                else:
                    response_text = str(responses).strip()
            else:
                # For seq2seq models, we use the full input as prompt
                responses = self.pipeline(
                    prompt,
                    max_length=max_len,
                    temperature=temp,
                    do_sample=True,
                    num_return_sequences=1
                )
                # Handle different response types
                if isinstance(responses, list):
                    if responses and isinstance(responses[0], dict) and 'generated_text' in responses[0]:
                        response_text = responses[0]['generated_text'].strip()
                    else:
                        response_text = str(responses[0]).strip()
                else:
                    response_text = str(responses).strip()
            
            return response_text
            
        except Exception as e:
            logger.error(f"Error generating response: {e}")
            return f"Error: Failed to generate response - {str(e)}"

# Global AI server instance
ai_server = None

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/v1/chat/completions', methods=['POST'])
def chat_completions():
    """OpenAI-compatible chat completions endpoint."""
    try:
        data = request.get_json()

        if not ai_server:
            return jsonify({
                "error": {
                    "message": "AI server is not initialized",
                    "type": "server_error",
                    "code": "internal_error"
                }
            }), 500
        
        # Extract parameters
        messages = data.get('messages', [])
        temperature = data.get('temperature', ai_server.temperature)
        max_tokens = data.get('max_tokens', ai_server.max_length)
        model = data.get('model', ai_server.model_name)
        
        # Convert messages to a single prompt
        prompt = ""
        for message in messages:
            role = message.get('role', 'user')
            content = message.get('content', '')
            
            if role == 'system':
                prompt += f"System: {content}\n"
            elif role == 'user':
                prompt += f"User: {content}\n"
            elif role == 'assistant':
                prompt += f"Assistant: {content}\n"
        
        # Add assistant prompt for response
        prompt += "Assistant:"
        
        logger.info(f"Generating response for prompt length: {len(prompt)} chars")
        
        # Generate response
        response_text = ai_server.generate_response(prompt, temperature, max_tokens)
        
        # Format as OpenAI-compatible response
        response = {
            "id": f"chatcmpl-{uuid.uuid4().hex[:10]}",
            "object": "chat.completion",
            "created": int(datetime.now().timestamp()),
            "model": model,
            "choices": [
                {
                    "index": 0,
                    "message": {
                        "role": "assistant",
                        "content": response_text
                    },
                    "finish_reason": "stop"
                }
            ],
            "usage": {
                "prompt_tokens": len(prompt.split()),
                "completion_tokens": len(response_text.split()),
                "total_tokens": len(prompt.split()) + len(response_text.split())
            }
        }
        
        return jsonify(response)
        
    except Exception as e:
        logger.error(f"Error in chat_completions: {e}")
        return jsonify({
            "error": {
                "message": str(e),
                "type": "server_error",
                "code": "internal_error"
            }
        }), 500

@app.route('/v1/models', methods=['GET'])
def list_models():
    """List available models endpoint."""
    if not ai_server:
        return jsonify({
            "error": {
                "message": "AI server is not initialized",
                "type": "server_error",
                "code": "internal_error"
            }
        }), 500
    
    return jsonify({
        "object": "list",
        "data": [
            {
                "id": ai_server.model_name,
                "object": "model",
                "created": int(datetime.now().timestamp()),
                "owned_by": "local",
                "permission": [],
                "root": ai_server.model_name,
                "parent": None
            }
        ]
    })

@app.route('/health', methods=['GET'])
def health_check():
    if not ai_server:
        return jsonify({
            "status": "unhealthy",
            "error": "AI server is not initialized"
        }), 500

    """Health check endpoint."""
    return jsonify({
        "status": "healthy",
        "model": ai_server.model_name,
        "model_type": ai_server.model_type,
        "timestamp": datetime.now().isoformat()
    })

@app.route('/test', methods=['POST'])
def test_generation():
    if not ai_server:
        return jsonify({
            "error": {
                "message": "AI server is not initialized",
                "type": "server_error",
                "code": "internal_error"
            }
        }), 500

    """Simple test endpoint for debugging."""
    try:
        data = request.get_json()
        prompt = data.get('prompt', 'Hello, how are you?')
        
        response = ai_server.generate_response(prompt)
        
        return jsonify({
            "prompt": prompt,
            "response": response,
            "model": ai_server.model_name
        })
        
    except Exception as e:
        return jsonify({"error": str(e)}), 500

def parse_args():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Flask server for Beta Evolve local AI integration",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python server.py microsoft/DialoGPT-medium
  python server.py google/flan-t5-large --port 8080
  python server.py microsoft/CodeGPT-small-py --temperature 0.3
  python server.py bigcode/starcoder --max-length 2048
        """
    )
    
    parser.add_argument(
        'model_name',
        help='HuggingFace model name (e.g., microsoft/DialoGPT-medium)'
    )
    
    parser.add_argument(
        '--port', '-p',
        type=int,
        default=5000,
        help='Port to run the server on (default: 5000)'
    )
    
    parser.add_argument(
        '--host',
        default='127.0.0.1',
        help='Host to bind the server to (default: 127.0.0.1)'
    )
    
    parser.add_argument(
        '--temperature', '-t',
        type=float,
        default=0.7,
        help='Default temperature for text generation (default: 0.7)'
    )
    
    parser.add_argument(
        '--max-length', '-m',
        type=int,
        default=1024,
        help='Maximum length for generated responses (default: 1024)'
    )
    
    parser.add_argument(
        '--debug',
        action='store_true',
        help='Enable debug mode'
    )
    
    return parser.parse_args()

def main():
    """Main function to start the server."""
    global ai_server
    
    # Parse command line arguments
    args = parse_args()
    
    # Validate model name
    if not args.model_name:
        print("Error: Please provide a HuggingFace model name")
        sys.exit(1)
    
    print(f"🤖 Beta Evolve Local AI Server")
    print(f"Model: {args.model_name}")
    print(f"Temperature: {args.temperature}")
    print(f"Max Length: {args.max_length}")
    print(f"Host: {args.host}:{args.port}")
    print("=" * 50)
    
    try:
        # Initialize the AI server
        ai_server = LocalAIServer(
            model_name=args.model_name,
            temperature=args.temperature,
            max_length=args.max_length
        )
        
        print(f"✅ Model loaded successfully!")
        print(f"🚀 Starting server on http://{args.host}:{args.port}")
        print("\nEndpoints:")
        print(f"  • Chat Completions: http://{args.host}:{args.port}/v1/chat/completions")
        print(f"  • Models List: http://{args.host}:{args.port}/v1/models")
        print(f"  • Health Check: http://{args.host}:{args.port}/health")
        print(f"  • Test: http://{args.host}:{args.port}/test")
        print("\nTo use with Beta Evolve, update your config.json:")
        print(f'  "fast_model_endpoint": "http://{args.host}:{args.port}/v1/chat/completions"')
        print(f'  "reasoning_model_endpoint": "http://{args.host}:{args.port}/v1/chat/completions"')
        print("=" * 50)
        
        # Start Flask server
        app.run(
            host=args.host,
            port=args.port,
            debug=args.debug,
            threaded=True
        )
        
    except KeyboardInterrupt:
        print("\n🛑 Server stopped by user")
    except Exception as e:
        print(f"❌ Error starting server: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()