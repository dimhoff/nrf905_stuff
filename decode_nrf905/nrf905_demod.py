#!/usr/bin/env python
##################################################
# Gnuradio Python Flow Graph
# Title: NRF905 Demodulator
# Author: David Imhoff <dimhoff_devel@xs4all.nl>
# Description: Demodulate the GFSK modulated signal send by a NRF905 transceiver
# Generated: Sun Jun 22 13:25:08 2014
##################################################

from gnuradio import blocks
from gnuradio import digital
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import wxgui
from gnuradio.eng_option import eng_option
from gnuradio.fft import window
from gnuradio.filter import firdes
from gnuradio.wxgui import fftsink2
from gnuradio.wxgui import forms
from grc_gnuradio import wxgui as grc_wxgui
from optparse import OptionParser
import osmosdr
import wx

class nrf905_demod(grc_wxgui.top_block_gui):

    def __init__(self):
        grc_wxgui.top_block_gui.__init__(self, title="NRF905 Demodulator")
        _icon_path = "/usr/share/icons/hicolor/32x32/apps/gnuradio-grc.png"
        self.SetIcon(wx.Icon(_icon_path, wx.BITMAP_TYPE_ANY))

        ##################################################
        # Variables
        ##################################################
        self.slider_rf_gain = slider_rf_gain = 30
        self.slider_if_gain = slider_if_gain = 30
        self.slider_freq = slider_freq = 868415e3
        self.samp_rate = samp_rate = 1000000
        self.checkbox_agc = checkbox_agc = True

        ##################################################
        # Blocks
        ##################################################
        _slider_rf_gain_sizer = wx.BoxSizer(wx.VERTICAL)
        self._slider_rf_gain_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_slider_rf_gain_sizer,
        	value=self.slider_rf_gain,
        	callback=self.set_slider_rf_gain,
        	label="RF Gain",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._slider_rf_gain_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_slider_rf_gain_sizer,
        	value=self.slider_rf_gain,
        	callback=self.set_slider_rf_gain,
        	minimum=0,
        	maximum=40,
        	num_steps=40,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_slider_rf_gain_sizer)
        _slider_if_gain_sizer = wx.BoxSizer(wx.VERTICAL)
        self._slider_if_gain_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_slider_if_gain_sizer,
        	value=self.slider_if_gain,
        	callback=self.set_slider_if_gain,
        	label="IF Gain",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._slider_if_gain_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_slider_if_gain_sizer,
        	value=self.slider_if_gain,
        	callback=self.set_slider_if_gain,
        	minimum=0,
        	maximum=56,
        	num_steps=56,
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_slider_if_gain_sizer)
        _slider_freq_sizer = wx.BoxSizer(wx.VERTICAL)
        self._slider_freq_text_box = forms.text_box(
        	parent=self.GetWin(),
        	sizer=_slider_freq_sizer,
        	value=self.slider_freq,
        	callback=self.set_slider_freq,
        	label="Frequency",
        	converter=forms.float_converter(),
        	proportion=0,
        )
        self._slider_freq_slider = forms.slider(
        	parent=self.GetWin(),
        	sizer=_slider_freq_sizer,
        	value=self.slider_freq,
        	callback=self.set_slider_freq,
        	minimum=863e6,
        	maximum=870e6,
        	num_steps=(8700-8630),
        	style=wx.SL_HORIZONTAL,
        	cast=float,
        	proportion=1,
        )
        self.Add(_slider_freq_sizer)
        self.notebook_0 = self.notebook_0 = wx.Notebook(self.GetWin(), style=wx.NB_TOP)
        self.notebook_0.AddPage(grc_wxgui.Panel(self.notebook_0), "FFT")
        self.notebook_0.AddPage(grc_wxgui.Panel(self.notebook_0), "scope")
        self.Add(self.notebook_0)
        self._checkbox_agc_check_box = forms.check_box(
        	parent=self.GetWin(),
        	value=self.checkbox_agc,
        	callback=self.set_checkbox_agc,
        	label="AGC",
        	true=True,
        	false=False,
        )
        self.Add(self._checkbox_agc_check_box)
        self.wxgui_fftsink2_0 = fftsink2.fft_sink_c(
        	self.notebook_0.GetPage(0).GetWin(),
        	baseband_freq=0,
        	y_per_div=10,
        	y_divs=10,
        	ref_level=0,
        	ref_scale=2.0,
        	sample_rate=samp_rate,
        	fft_size=1024,
        	fft_rate=15,
        	average=False,
        	avg_alpha=None,
        	title="FFT Plot",
        	peak_hold=False,
        )
        self.notebook_0.GetPage(0).Add(self.wxgui_fftsink2_0.win)
        self.rtlsdr_source_0 = osmosdr.source( args="numchan=" + str(1) + " " + "" )
        self.rtlsdr_source_0.set_sample_rate(samp_rate)
        self.rtlsdr_source_0.set_center_freq(slider_freq, 0)
        self.rtlsdr_source_0.set_freq_corr(0, 0)
        self.rtlsdr_source_0.set_dc_offset_mode(0, 0)
        self.rtlsdr_source_0.set_iq_balance_mode(0, 0)
        self.rtlsdr_source_0.set_gain_mode(checkbox_agc, 0)
        self.rtlsdr_source_0.set_gain(slider_rf_gain, 0)
        self.rtlsdr_source_0.set_if_gain(slider_if_gain, 0)
        self.rtlsdr_source_0.set_bb_gain(20, 0)
        self.rtlsdr_source_0.set_antenna("", 0)
        self.rtlsdr_source_0.set_bandwidth(0, 0)
          
        self.digital_gfsk_demod_0 = digital.gfsk_demod(
        	samples_per_symbol=int(samp_rate/100e3),
        	sensitivity=1.0,
        	gain_mu=0.175,
        	mu=0.5,
        	omega_relative_limit=0.005,
        	freq_error=0.0,
        	verbose=False,
        	log=False,
        )
        self.blocks_file_sink_0 = blocks.file_sink(gr.sizeof_char*1, "/tmp/nrf.dat", False)
        self.blocks_file_sink_0.set_unbuffered(False)

        ##################################################
        # Connections
        ##################################################
        self.connect((self.rtlsdr_source_0, 0), (self.digital_gfsk_demod_0, 0))
        self.connect((self.digital_gfsk_demod_0, 0), (self.blocks_file_sink_0, 0))
        self.connect((self.rtlsdr_source_0, 0), (self.wxgui_fftsink2_0, 0))


# QT sink close method reimplementation

    def get_slider_rf_gain(self):
        return self.slider_rf_gain

    def set_slider_rf_gain(self, slider_rf_gain):
        self.slider_rf_gain = slider_rf_gain
        self._slider_rf_gain_slider.set_value(self.slider_rf_gain)
        self._slider_rf_gain_text_box.set_value(self.slider_rf_gain)
        self.rtlsdr_source_0.set_gain(self.slider_rf_gain, 0)

    def get_slider_if_gain(self):
        return self.slider_if_gain

    def set_slider_if_gain(self, slider_if_gain):
        self.slider_if_gain = slider_if_gain
        self._slider_if_gain_slider.set_value(self.slider_if_gain)
        self._slider_if_gain_text_box.set_value(self.slider_if_gain)
        self.rtlsdr_source_0.set_if_gain(self.slider_if_gain, 0)

    def get_slider_freq(self):
        return self.slider_freq

    def set_slider_freq(self, slider_freq):
        self.slider_freq = slider_freq
        self._slider_freq_slider.set_value(self.slider_freq)
        self._slider_freq_text_box.set_value(self.slider_freq)
        self.rtlsdr_source_0.set_center_freq(self.slider_freq, 0)

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.wxgui_fftsink2_0.set_sample_rate(self.samp_rate)
        self.rtlsdr_source_0.set_sample_rate(self.samp_rate)

    def get_checkbox_agc(self):
        return self.checkbox_agc

    def set_checkbox_agc(self, checkbox_agc):
        self.checkbox_agc = checkbox_agc
        self._checkbox_agc_check_box.set_value(self.checkbox_agc)
        self.rtlsdr_source_0.set_gain_mode(self.checkbox_agc, 0)

if __name__ == '__main__':
    import ctypes
    import os
    if os.name == 'posix':
        try:
            x11 = ctypes.cdll.LoadLibrary('libX11.so')
            x11.XInitThreads()
        except:
            print "Warning: failed to XInitThreads()"
    parser = OptionParser(option_class=eng_option, usage="%prog: [options]")
    (options, args) = parser.parse_args()
    tb = nrf905_demod()
    tb.Start(True)
    tb.Wait()

