from dataclasses import dataclass, asdict
from layer_info import Conv2DShapeParam, MaxPool2DShapeParam
import math
from math import ceil

# Memory
DATA_SIZE = 1  # Byte
PSUM_DATA_SIZE = 4  # Byte
BUS_BANDWIDTH = 4  # Byte

# Time
CLOCK_RATE = 200 * 1e6  # 200 MHz
TIME_UNIT = 1  # cycle
SPAD_ACCESS_TIME = 1 * TIME_UNIT
GLB_ACCESS_TIME = 2 * TIME_UNIT
DRAM_ACCESS_TIME = 5 * TIME_UNIT

# Energy
ENERGY_UNIT = 1e-6  # 1 pJ = 10^6 uJ
ENERGY_PER_MAC = 2 * ENERGY_UNIT
ENERGY_PER_GLB_ACCESS = 10 * ENERGY_UNIT
ENERGY_PER_DRAM_ACCESS = 200 * ENERGY_UNIT
POWER_UNIT = 1  # 1 uW
POWER_LEAKAGE = 50 * POWER_UNIT

######################################################################################################
# N: number of ifmaps/ofmaps
# M: number of filters
# H/W: ifmap height/width
# R/S: filter height/width
# E/F: ofmap height/width
# U: stride
#  ----------------------------------------------------------------------------------------------
# m: ofmap channels in global buffer
# n: number of ifmaps in a pass
# e: width of PE-set
# p: number of filters in a pass
# q: (ifmap or filter) channels in a pass
# r: number of PE sets for different (ifmap/filter) channels
# t: number of PE sets for different filters
#  ----------------------------------------------------------------------------------------------
#  Naming Convention
# *_per_pass: compute / storage size required per pass
# *_per_layer: compute / storage size required per layer
######################################################################################################


@dataclass
class EyerissHardwareParam:
    pe_array_h: int
    pe_array_w: int
    ifmap_spad_size: int
    filter_spad_size: int
    psum_spad_size: int
    glb_size: int
    bus_bw: int
    noc_bw: int


@dataclass
class EyerissMappingParam:
    m: int  # number of ofmap channels stored in global buffer
    n: int  # number of ofmaps/ifmaps used in a processing pass
    e: int  # width of the PE set (strip-mined if nessary)
    p: int  # number of filters processed by a PE set
    q: int  # number of ifmap/filter channels processed by a PE set
    r: int  # number of PE sets for different ifmap/filter channels
    t: int  # number of PE sets for different filters


AnalysisResult = dict[str, str | int | float]


class EyerissAnalyzer:
    cnt = 0

    def __init__(
        self,
        name: str | None = None,
        hardware_param: EyerissHardwareParam | None = None,
    ) -> None:
        self.name = name if name is not None else f"mapping_{EyerissAnalyzer.cnt}"
        self._hardware = hardware_param
        self._conv_shape = None
        self._maxpool_shape = None
        self._mapping = None
        EyerissAnalyzer.cnt += 1

    @property
    def hardware(self) -> EyerissHardwareParam:
        return self._hardware

    @hardware.setter
    def hardware(self, hardware_param: EyerissHardwareParam) -> None:
        assert isinstance(hardware_param, EyerissHardwareParam)
        self._hardware = hardware_param

    @property
    def conv_shape(self) -> Conv2DShapeParam:
        return self._conv_shape

    @conv_shape.setter
    def conv_shape(self, conv_param: Conv2DShapeParam) -> None:
        assert isinstance(conv_param, Conv2DShapeParam)
        self._conv_shape = conv_param

    @property
    def maxpool_shape(self) -> MaxPool2DShapeParam:
        return self._maxpool_shape

    @maxpool_shape.setter
    def maxpool_shape(self, maxpool_param: MaxPool2DShapeParam | None) -> None:
        assert isinstance(maxpool_param, (MaxPool2DShapeParam, type(None)))
        self._maxpool_shape = maxpool_param

    @property
    def mapping(self) -> EyerissMappingParam:
        return self._mapping

    @mapping.setter
    def mapping(self, mapping_param: EyerissMappingParam) -> None:
        self._mapping = mapping_param

    # Scratchpad Memory Usage
    def filter_used(self) -> int:
        return self.mapping.q * self.conv_shape.S * self.mapping.p

    def ifmap_used(self) -> int:
        return self.mapping.q * self.conv_shape.S

    def psum_used(self) -> int:
        return self.mapping.p

    @property
    def spad_size_legal(self) -> dict[str, bool]:
        return {
            "ifmap": self.ifmap_used() <= self.hardware.ifmap_spad_size,
            "filter": self.filter_used() <= self.hardware.filter_spad_size,
            "psum": self.psum_used() <= self.hardware.psum_spad_size,
        }

    # Global Buffer (GLB) Usage
    @property
    def glb_usage_per_pass(self) -> dict[str, int]:
        sizes: dict[str, int] = {}
        n, q, r, e = self.mapping.n, self.mapping.q, self.mapping.r, self.mapping.e
        U, R, S, p, t = self.conv_shape.U, self.conv_shape.R, self.conv_shape.S, self.mapping.p, self.mapping.t
        m, F, W = self.conv_shape.M, self.conv_shape.F, self.conv_shape.W

        glb_ifmap = n * q * r * (U * (e - 1) + R) * W * DATA_SIZE
        glb_filter = p * t * q * r * R * S * DATA_SIZE
        glb_bias = p * t * PSUM_DATA_SIZE
        glb_psum = n * m * e * F * PSUM_DATA_SIZE

        sizes = {
            "ifmap": glb_ifmap,
            "filter": glb_filter,
            "bias": glb_bias,
            "psum": glb_psum,
            "total": glb_ifmap + glb_filter + glb_bias + glb_psum
        }

        return sizes

    
    @property
    def glb_size_legal(self) -> bool:
        return self.glb_usage_per_pass["total"] <= self.hardware.glb_size

    # DRAM Accesses (DRAM-GLB data movement)
    @property
    def dram_access_per_layer(self) -> dict[str, int]:
        res: dict[str, int] = {}
        N, H, W, C, M = self.conv_shape.N, self.conv_shape.H, self.conv_shape.W, self.conv_shape.C, self.conv_shape.M
        R, S, E, F, U, P = self.conv_shape.R, self.conv_shape.S, self.conv_shape.E, self.conv_shape.F, self.conv_shape.U, self.conv_shape.P
        m, n, e, p, q, r, t = self.mapping.m, self.mapping.n, self.mapping.e, self.mapping.p, self.mapping.q, self.mapping.r, self.mapping.t

        dram_ifmap = (
            ceil(M / m) *
            ceil(E / e) *
            ceil(N / n) *
            ceil(C / (q * r)) *
            n * q * r * (U * (e - 1) + R) * W * DATA_SIZE
        )

        dram_filter = (
            ceil(E / e) *
            ceil(N / n) *
            ceil(M / m) *
            ceil(C / (q * r)) *
            ceil(m / (p * t)) *
            p * t * q * r * R * S * DATA_SIZE
        )

        dram_bias = (
            ceil(E / e) *
            ceil(N / n) *
            ceil(M / m) *
            m * PSUM_DATA_SIZE
        )

        dram_ofmap = (
            ceil(M / m) *
            ceil(E / e) *
            ceil(N / n) *
            n * m * e * F * DATA_SIZE
        )

        if self.maxpool_shape:
            dram_ofmap = dram_ofmap // 4

        res = {
            "ifmap_read": dram_ifmap,
            "filter_read": dram_filter,
            "bias_read": dram_bias,
            "ofmap_write": dram_ofmap,
            "read": dram_ifmap + dram_filter + dram_bias,
            "write": dram_ofmap,
            "total": dram_ifmap + dram_filter + dram_bias + dram_ofmap
        }

        return res

    # GLB Accesses (GLB-Spad data movement)
    @property
    def glb_access_per_layer(self) -> dict[str, int]:
        res: dict[str, int] = {}
        N, H, W, C, M = self.conv_shape.N, self.conv_shape.H, self.conv_shape.W, self.conv_shape.C, self.conv_shape.M
        R, S, E, F, U, P = self.conv_shape.R, self.conv_shape.S, self.conv_shape.E, self.conv_shape.F, self.conv_shape.U, self.conv_shape.P
        m, n, e, p, q, r, t = self.mapping.m, self.mapping.n, self.mapping.e, self.mapping.p, self.mapping.q, self.mapping.r, self.mapping.t
    
        glb_ifmap = (
            ceil(M / m) *
            ceil(E / e) *
            ceil(N / n) *
            ceil(C / (q * r)) *
            ceil(m / (p * t)) *
            n * q * r * (U * (e - 1) + R) * W * DATA_SIZE
        )
    
        glb_filter = (
            ceil(E / e) *
            ceil(N / n) *
            ceil(M / m) *
            ceil(C / (q * r)) *
            ceil(m / (p * t)) *
            p * t * q * r * R * S * DATA_SIZE
        )
    
        glb_bias = (
            ceil(E / e) *
            ceil(N / n) *
            ceil(M / m) *
            m * PSUM_DATA_SIZE
        )

        if self.conv_shape:
            glb_psum_read = (
                ceil(M / m) *
                ceil(E / e) *
                ceil(N / n) *
                (ceil(C / (q * r)) - 1) *
                n * m * e * F * PSUM_DATA_SIZE
            )
    
            glb_psum_write = (
                ceil(M / m) *
                ceil(E / e) *
                ceil(N / n) *
                ceil(C / (q * r)) *  
                n * m * e * F * PSUM_DATA_SIZE
            )
        else:
            glb_psum_read = (
                ceil(M / m) *
                ceil(E / e) *
                ceil(N / n) *
                n * m * e * F * PSUM_DATA_SIZE
            )

            glb_psum_write = (
                ceil(M / m) *
                ceil(E / e) *
                ceil(N / n) *
                n * m * e * F * PSUM_DATA_SIZE
            )

            if self.maxpool_shape:
                glb_psum_write = glb_psum_write // 4


             

        res = {
            "ifmap_read": glb_ifmap,
            "filter_read": glb_filter,
            "bias_read": glb_bias,
            "psum_read": glb_psum_read,
            "psum_write": glb_psum_write,
            "read": glb_ifmap + glb_filter + glb_bias + glb_psum_read,
            "write": glb_psum_write,
            "total": glb_ifmap + glb_filter + glb_bias + glb_psum_read + glb_psum_write
        }
    
        return res

    
    @property
    def latency_per_layer(self) -> int:
        ofmap_size = (
            self.conv_shape.N
            * self.conv_shape.M
            * self.conv_shape.E
            * self.conv_shape.F
        )
        ppu_latency_per_elem = 1 if self.maxpool_shape is None else 5

        return (
            self.glb_access_per_layer["total"] * GLB_ACCESS_TIME
            + self.dram_access_per_layer["total"] * DRAM_ACCESS_TIME
            + ofmap_size * ppu_latency_per_elem
        )

    @property
    def macs_per_layer(self) -> int:
        return (
            self.conv_shape.N
            * self.conv_shape.M
            * self.conv_shape.E
            * self.conv_shape.F
            * self.conv_shape.C
            * self.conv_shape.R
            * self.conv_shape.S
        )

    @property
    def energy_per_layer(self) -> dict[str, float]:
        compute_energy = self.macs_per_layer * ENERGY_PER_MAC
        memory_energy = (
            self.glb_access_per_layer["total"] * ENERGY_PER_GLB_ACCESS
            + self.dram_access_per_layer["total"] * ENERGY_PER_DRAM_ACCESS
        )
        leakage_energy = POWER_LEAKAGE * self.latency_per_layer / CLOCK_RATE
        total_energy = compute_energy + memory_energy + leakage_energy
        return {
            "compute": compute_energy,
            "memory": memory_energy,
            "leakage": leakage_energy,
            "total": total_energy,
        }

    @property
    def power_per_layer(self) -> dict[str, float]:
        compute_power = (
            self.energy_per_layer["compute"] / self.latency_per_layer * CLOCK_RATE
        )
        memory_power = (
            self.energy_per_layer["memory"] / self.latency_per_layer * CLOCK_RATE
        )
        leakage_power = POWER_LEAKAGE
        total_power = compute_power + memory_power + leakage_power
        return {
            "compute": compute_power,
            "memory": memory_power,
            "leakage": leakage_power,
            "total": total_power,
        }

    @property
    def operational_intensity(self) -> float:
        return self.macs_per_layer / self.dram_access_per_layer["total"]

    @property
    def peak_performance(self) -> float:
        return self.hardware.pe_array_h * self.hardware.pe_array_w  # MACs per cycle

    @property
    def peak_bandwidth(self) -> float:
        return self.hardware.bus_bw  # bytes per cycle

    @property
    def bound_by(self) -> str:
        machine_blance_point = self.peak_performance / self.peak_bandwidth
        if self.operational_intensity > machine_blance_point:
            return "compute"
        elif self.operational_intensity < machine_blance_point:
            return "memory"
        else:
            return "balanced"

    @property
    def is_compute_bound(self) -> bool:
        return self.bound_by == "compute"

    @property
    def is_memory_bound(self) -> bool:
        return self.bound_by == "memory"

    @property
    def is_balanced(self) -> bool:
        return self.bound_by == "balanced"

    @property
    def summary(self) -> AnalysisResult:
        return {
            "layer": self.name,
            **asdict(self.hardware),
            **asdict(self.mapping),
            "glb_usage": self.glb_usage_per_pass["total"],  # bytes
            "glb_access": self.glb_access_per_layer["total"],  # bytes
            "dram_access": self.dram_access_per_layer["total"],  # bytes
            "macs": self.macs_per_layer,
            "latency": self.latency_per_layer,  # cycles
            "operational_intensity": self.operational_intensity,
            "peak_performance": self.peak_performance,
            "peak_bandwidth": self.peak_bandwidth,
            "bound": self.bound_by
        }
