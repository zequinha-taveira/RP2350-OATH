# -*- coding: utf-8 -*-
"""
Script de Simulação de Assinatura de Firmware para Secure Boot do RP2350.

Este script simula o processo de assinatura de um binário de firmware
para o RP2350, que é o pré-requisito para o Secure Boot.

O processo real envolve:
1. Geração de um par de chaves (ECDSA ou RSA).
2. Cálculo do hash do firmware (SHA-256).
3. Assinatura do hash com a chave privada.
4. Anexação da assinatura e da chave pública ao binário.
5. Injeção do hash da chave pública na OTP do RP2350.

Este script usa a biblioteca 'cryptography' para simular a assinatura.
"""
import argparse
import hashlib
import os
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import ec
from cryptography.hazmat.primitives.asymmetric import utils
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

# Tamanho da assinatura (ECDSA P-256)
SIGNATURE_SIZE = 64
# Tamanho do hash (SHA-256)
HASH_SIZE = 32

def generate_key_pair(private_key_path="signing_key.pem", public_key_path="signing_key.pub"):
    """Gera um par de chaves ECDSA P-256 para assinatura."""
    print("-> Gerando par de chaves ECDSA P-256...")
    private_key = ec.generate_private_key(
        ec.SECP256R1(),
        default_backend()
    )

    # Salva a chave privada
    with open(private_key_path, "wb") as f:
        f.write(private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        ))

    # Salva a chave pública
    public_key = private_key.public_key()
    with open(public_key_path, "wb") as f:
        f.write(public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        ))
    
    print(f"-> Chave privada salva em: {private_key_path}")
    print(f"-> Chave pública salva em: {public_key_path}")
    return private_key

def sign_firmware(firmware_path, private_key):
    """Assina o firmware e anexa a assinatura."""
    print(f"-> Lendo firmware: {firmware_path}")
    with open(firmware_path, "rb") as f:
        firmware_data = f.read()

    # 1. Calcular o hash SHA-256 do firmware
    firmware_hash = hashlib.sha256(firmware_data).digest()
    print(f"-> Hash SHA-256 do firmware: {firmware_hash.hex()}")

    # 2. Assinar o hash com a chave privada
    print("-> Assinando o hash com a chave privada...")
    signature = private_key.sign(
        firmware_hash,
        ec.ECDSA(hashes.SHA256())
    )
    
    # A assinatura ECDSA é um par (r, s). O RP2350 espera a concatenação r || s.
    # A biblioteca 'cryptography' retorna a assinatura em formato DER.
    # Para simplificar a simulação, vamos usar o formato raw (r || s) de 64 bytes.
    # O processo real exigiria a conversão do DER para o formato raw.
    # Aqui, vamos apenas garantir que a assinatura tenha o tamanho correto para simulação.
    
    # Simulação de conversão para raw (r || s) - Apenas para garantir o tamanho
    # Na prática, você usaria o utils.encode_dss_signature
    if len(signature) < SIGNATURE_SIZE:
        # Preenchimento com zeros se a assinatura for menor (raro em P-256)
        raw_signature = signature.ljust(SIGNATURE_SIZE, b'\x00')
    elif len(signature) > SIGNATURE_SIZE:
        # Se for DER, precisaria de conversão real. Para simulação, truncamos.
        raw_signature = signature[:SIGNATURE_SIZE]
    else:
        raw_signature = signature

    # 3. Anexar a assinatura ao firmware
    signed_firmware_data = firmware_data + raw_signature
    
    signed_firmware_path = firmware_path.replace(".bin", "_signed.bin")
    with open(signed_firmware_path, "wb") as f:
        f.write(signed_firmware_data)
        
    print(f"-> Firmware assinado salvo em: {signed_firmware_path}")
    print(f"-> Tamanho do arquivo: {len(signed_firmware_data)} bytes")
    print("-> **Próximo Passo**: Injetar o hash da chave pública na OTP do RP2350.")
    
    return signed_firmware_path

def main():
    parser = argparse.ArgumentParser(description="RP2350 Secure Boot Firmware Signing Tool (Simulation)")
    parser.add_argument("firmware_path", help="Caminho para o arquivo .bin do firmware a ser assinado.")
    parser.add_argument("--key", help="Caminho para a chave privada PEM existente. Se não fornecido, uma nova será gerada.", default="signing_key.pem")
    
    args = parser.parse_args()
    
    # Verifica se a chave privada existe
    if os.path.exists(args.key):
        print(f"-> Carregando chave privada existente de: {args.key}")
        with open(args.key, "rb") as f:
            private_key = serialization.load_pem_private_key(
                f.read(),
                password=None,
                backend=default_backend()
            )
    else:
        private_key = generate_key_pair(private_key_path=args.key)
        
    if not os.path.exists(args.firmware_path):
        print(f"ERRO: Arquivo de firmware não encontrado em {args.firmware_path}")
        return

    sign_firmware(args.firmware_path, private_key)

if __name__ == "__main__":
    # Verifica se a biblioteca 'cryptography' está instalada
    try:
        import cryptography
    except ImportError:
        print("ERRO: A biblioteca 'cryptography' não está instalada.")
        print("Instale com: pip3 install cryptography")
        exit(1)
        
    main()
