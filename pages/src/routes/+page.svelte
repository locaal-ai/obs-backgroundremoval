<script>
	import { base } from '$app/paths';
</script>

<svelte:head>
	<title>OBS Background Removal</title>
	<meta
		name="description"
		content="OBS Plugin: Background Removal / Virtual Green-screen and Low-Light Enhancement"
	/>
</svelte:head>

<section>
	<header>
		<h1>OBS Background Removal</h1>
		<h2>
			OBS Plugin: Background Removal / Virtual Green-screen and Low-Light Enhancement
		</h2>
	</header>

    <p>See our <a href="{base}/usage">Usage</a> page for instructions on usage in OBS</p>

	<h1>Download and Install (Official)</h1>
	<a href="{base}/windows"><h2>Windows</h2></a>
	<a href="{base}/macos"><h2>macOS</h2></a>
	<a href="{base}/ubuntu"><h2>Ubuntu Linux</h2></a>
	<a href="{base}/flatpak"><h2>FlatPak</h2></a>

	<h1>Download and Install (Unofficial)</h1>
	<a href="{base}/arch"><h2>Arch Linux</h2></a>
	<a href="{base}/opensuse-tumbleweed"><h2>openSUSE Tumbleweed</h2></a>
	<a href="{base}/fedora"><h2>Fedora</h2></a>
</section>

<style>
	section {
		display: flex;
		flex-direction: column;
		justify-content: center;
		align-items: center;
		flex: 0.6;
	}

	h1 {
		width: 100%;
	}
</style>
