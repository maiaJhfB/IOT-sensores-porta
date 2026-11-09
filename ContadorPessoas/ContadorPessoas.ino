#include <Arduino.h>

constexpr uint8_t PINOS_TRIG[] = {4, 6};
constexpr uint8_t PINOS_ECHO[] = {5, 7};

// A fica do lado de fora e B do lado de dentro da sala monitorada.
constexpr uint8_t LIVRE = 0;
constexpr uint8_t ZONA_A = 1;
constexpr uint8_t ZONA_B = 2;
constexpr uint8_t AMBAS = ZONA_A | ZONA_B;

constexpr uint32_t INTERVALO_DISPARO_MS = 40;  // Cada sensor dispara a cada 80 ms.
constexpr uint32_t TIMEOUT_ECO_US = 26000;
constexpr uint32_t TIMEOUT_TRAVESSIA_MS = 3000;
constexpr uint16_t DISTANCIA_MAX_MM = 4000;
constexpr uint8_t AMOSTRAS_CALIBRACAO = 20;
constexpr uint16_t VARIACAO_MAX_FUNDO_MM = 60;
constexpr int16_t LIMIAR_OCUPAR_MM = 180;
constexpr int16_t LIMIAR_LIBERAR_MM = 100;
constexpr uint8_t PARES_LIVRES = 2;
constexpr bool MOSTRAR_LEITURAS = false;

enum class Etapa : uint8_t {
  AguardandoLivre,
  Pronto,
  PrimeiraZona,
  DuasZonas,
  UltimaZona
};

uint16_t fundoMm[2] = {};
bool ocupado[2] = {};
Etapa etapa = Etapa::AguardandoLivre;
uint8_t primeiraZona = LIVRE;
uint8_t paresLivres = 0;
uint32_t inicioTravessiaMs = 0;
uint32_t entradas = 0;
uint32_t saidas = 0;

uint16_t medirDistanciaMm(uint8_t zona) {
  static uint32_t ultimoDisparoMs = millis() - INTERVALO_DISPARO_MS;
  const uint32_t decorridoMs = millis() - ultimoDisparoMs;
  if (decorridoMs < INTERVALO_DISPARO_MS) {
    delay(INTERVALO_DISPARO_MS - decorridoMs);
  }

  ultimoDisparoMs = millis();
  digitalWrite(PINOS_TRIG[zona], HIGH);
  delayMicroseconds(10);
  digitalWrite(PINOS_TRIG[zona], LOW);

  const unsigned long pulsoUs = pulseIn(PINOS_ECHO[zona], HIGH, TIMEOUT_ECO_US);
  const unsigned long distanciaMm = (pulsoUs * 10 + 29) / 58;
  if (distanciaMm < 20 || distanciaMm > DISTANCIA_MAX_MM) {
    return 0;  // Sem eco ou fora da faixa válida.
  }
  return static_cast<uint16_t>(distanciaMm);
}

bool calibrar() {
  uint32_t soma[2] = {};
  uint16_t menor[2] = {DISTANCIA_MAX_MM, DISTANCIA_MAX_MM};
  uint16_t maior[2] = {};

  for (uint8_t amostra = 0; amostra < AMOSTRAS_CALIBRACAO; ++amostra) {
    for (uint8_t zona = 0; zona < 2; ++zona) {
      const uint16_t distancia = medirDistanciaMm(zona);
      if (distancia == 0) return false;
      soma[zona] += distancia;
      if (distancia < menor[zona]) menor[zona] = distancia;
      if (distancia > maior[zona]) maior[zona] = distancia;
    }
  }

  for (uint8_t zona = 0; zona < 2; ++zona) {
    fundoMm[zona] = soma[zona] / AMOSTRAS_CALIBRACAO;
    if (fundoMm[zona] < 300 || fundoMm[zona] > 3000 ||
        maior[zona] - menor[zona] > VARIACAO_MAX_FUNDO_MM) {
      return false;
    }
  }
  return true;
}

void descartarTravessia(const __FlashStringHelper *motivo) {
  Serial.print(F("Travessia descartada - "));
  Serial.println(motivo);
  etapa = Etapa::AguardandoLivre;
  paresLivres = 0;
}

void registrarTravessia() {
  if (primeiraZona == ZONA_A) {
    ++entradas;
    Serial.print(F("ENTRADA"));
  } else {
    ++saidas;
    Serial.print(F("SAIDA"));
  }
  Serial.print(F(" | Entradas: "));
  Serial.print(entradas);
  Serial.print(F(" | Saidas: "));
  Serial.println(saidas);
}

void processarZonas(bool leituraValida, uint8_t zonas, uint32_t agoraMs) {
  const bool emTravessia = etapa == Etapa::PrimeiraZona ||
                          etapa == Etapa::DuasZonas ||
                          etapa == Etapa::UltimaZona;
  if (!leituraValida) {
    if (emTravessia) {
      descartarTravessia(F("eco invalido"));
    } else {
      etapa = Etapa::AguardandoLivre;
      paresLivres = 0;
    }
    return;
  }

  if (emTravessia && agoraMs - inicioTravessiaMs >= TIMEOUT_TRAVESSIA_MS) {
    descartarTravessia(F("tempo de travessia excedido"));
    return;
  }

  // Caminho completo de entrada: livre, A, A+B, B, livre.
  // A saída usa o mesmo caminho com A e B trocados.
  const uint8_t ultimaZona = AMBAS ^ primeiraZona;
  switch (etapa) {
    case Etapa::AguardandoLivre:
      if (zonas != LIVRE) {
        paresLivres = 0;
      } else if (++paresLivres >= PARES_LIVRES) {
        etapa = Etapa::Pronto;
      }
      break;

    case Etapa::Pronto:
      if (zonas == AMBAS) {
        descartarTravessia(F("ordem inicial indefinida"));
      } else if (zonas != LIVRE) {
        primeiraZona = zonas;
        inicioTravessiaMs = agoraMs;
        paresLivres = 0;
        etapa = Etapa::PrimeiraZona;
      }
      break;

    case Etapa::PrimeiraZona:
      if (zonas == AMBAS) {
        etapa = Etapa::DuasZonas;
      } else if (zonas != primeiraZona) {
        descartarTravessia(F("travessia interrompida ou sem sobreposicao"));
      }
      break;

    case Etapa::DuasZonas:
      if (zonas == ultimaZona) {
        etapa = Etapa::UltimaZona;
      } else if (zonas != AMBAS) {
        descartarTravessia(F("retorno ou sequencia incompleta"));
      }
      break;

    case Etapa::UltimaZona:
      if (zonas == LIVRE) {
        if (++paresLivres >= PARES_LIVRES) {
          registrarTravessia();
          etapa = Etapa::Pronto;
        }
      } else if (zonas == ultimaZona) {
        paresLivres = 0;
      } else {
        descartarTravessia(F("retorno ou sobreposicao de passagens"));
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);
  for (uint8_t zona = 0; zona < 2; ++zona) {
    pinMode(PINOS_TRIG[zona], OUTPUT);
    digitalWrite(PINOS_TRIG[zona], LOW);
    pinMode(PINOS_ECHO[zona], INPUT);
  }

  Serial.println(F("Calibrando. Mantenha a passagem vazia."));
  while (!calibrar()) {
    Serial.println(F("Fundo invalido ou instavel. Verifique alinhamento e ligacoes."));
    delay(1000);
  }

  Serial.print(F("Calibrado | Fundo A (mm): "));
  Serial.print(fundoMm[0]);
  Serial.print(F(" | Fundo B (mm): "));
  Serial.println(fundoMm[1]);
  Serial.println(F("Entradas: 0 | Saidas: 0"));
}

void loop() {
  const uint16_t distancias[] = {medirDistanciaMm(0), medirDistanciaMm(1)};
  const bool leituraValida = distancias[0] != 0 && distancias[1] != 0;
  uint8_t zonas = LIVRE;
  if (leituraValida) {
    for (uint8_t zona = 0; zona < 2; ++zona) {
      const int16_t aproximacao =
          static_cast<int16_t>(fundoMm[zona]) - static_cast<int16_t>(distancias[zona]);
      const int16_t limiar = ocupado[zona] ? LIMIAR_LIBERAR_MM : LIMIAR_OCUPAR_MM;
      ocupado[zona] = aproximacao >= limiar;
    }
    zonas = (ocupado[0] ? ZONA_A : LIVRE) | (ocupado[1] ? ZONA_B : LIVRE);
  }

  const uint32_t agoraMs = millis();
  processarZonas(leituraValida, zonas, agoraMs);
  if (MOSTRAR_LEITURAS) {
    Serial.print(agoraMs);
    Serial.print(F(" ms | A (mm): "));
    Serial.print(distancias[0]);
    Serial.print(F(" | B (mm): "));
    Serial.print(distancias[1]);
    Serial.print(F(" | Zonas: "));
    if (leituraValida) {
      Serial.println(zonas);
    } else {
      Serial.println(F("?"));
    }
  }
}
